#include "database.h"
#include "homfly_backend.h"
#include "khovanov_backend.h"
#include "pd_code.h"
#include "process_runner.h"
#include "runtime_control.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#else
#include <limits.h>
#include <unistd.h>
#endif

namespace hki {

namespace {

struct Options {
    std::string pdText;
    std::filesystem::path dataRoot;
    int timeoutSeconds = 60;
    bool verbose = false;
    bool printInvariants = false;
};

struct DataPaths {
    std::filesystem::path homflyDb;
    std::filesystem::path khovanovDb;
    std::filesystem::path normalizerDir;
};

std::string readWholeFile(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("cannot open file: " + path.string());
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

void writeWholeFile(const std::filesystem::path& path, const std::string& value) {
    std::ofstream out(path, std::ios::binary);
    if (!out) throw std::runtime_error("cannot write file: " + path.string());
    out << value;
}

void usage(std::ostream& out) {
    out << "Usage:\n"
        << "  cpp_knot_indexer --pd-code \"[[1,5,2,4],...]\" [--timeout SEC]\n"
        << "  cpp_knot_indexer --pd-file code.txt [--timeout SEC]\n"
        << "  cpp_knot_indexer < code.txt\n\n"
        << "Options:\n"
        << "  --timeout SEC       Max seconds for HOMFLY-PT and Khovanov each (0 disables timeout; default 60).\n"
        << "  --data-root PATH    Project root containing the data/ directory.\n"
        << "  --print-invariants  Print computed invariant strings to stderr.\n"
        << "  --verbose           Print worker status, failures, and invariant strings to stderr.\n";
}

std::filesystem::path currentExecutablePath(const char* argv0) {
#ifdef _WIN32
    std::wstring buffer(32768, L'\0');
    DWORD n = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (n > 0 && n < buffer.size()) {
        buffer.resize(n);
        return std::filesystem::path(buffer);
    }
#elif defined(__APPLE__)
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::string buffer(size + 1, '\0');
    if (_NSGetExecutablePath(buffer.data(), &size) == 0) {
        char resolved[PATH_MAX];
        if (realpath(buffer.c_str(), resolved)) return std::filesystem::path(resolved);
        return std::filesystem::path(buffer.c_str());
    }
#else
    char buffer[PATH_MAX];
    ssize_t n = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (n > 0) {
        buffer[n] = '\0';
        return std::filesystem::path(buffer);
    }
#endif
    return std::filesystem::absolute(argv0 ? argv0 : "");
}

bool existsPath(const std::filesystem::path& path) {
    std::error_code ec;
    return std::filesystem::exists(path, ec);
}

std::optional<DataPaths> tryResolveDataPaths(const std::filesystem::path& base) {
    const std::filesystem::path hom =
        "data/homfly/sorted_HOMFLY-PT.txt";
    const std::filesystem::path kho =
        "data/khovanov/sorted_khovanov.txt";
    const std::filesystem::path norm =
        "data/knotname-reg";

    DataPaths paths{base / hom, base / kho, base / norm};
    if (existsPath(paths.homflyDb) && existsPath(paths.khovanovDb) && existsPath(paths.normalizerDir)) return paths;
    return std::nullopt;
}

DataPaths findDataPaths(const std::filesystem::path& executable, const std::filesystem::path& userRoot) {
    std::vector<std::filesystem::path> starts;
    if (!userRoot.empty()) starts.push_back(userRoot);
    starts.push_back(std::filesystem::current_path());
    starts.push_back(executable.parent_path());

    for (std::filesystem::path start : starts) {
        std::error_code ec;
        start = std::filesystem::absolute(start, ec);
        if (ec) continue;
        while (!start.empty()) {
            if (auto paths = tryResolveDataPaths(start)) return *paths;
            std::filesystem::path parent = start.parent_path();
            if (parent == start) break;
            start = parent;
        }
    }
    throw std::runtime_error("cannot locate invariant databases; pass --data-root");
}

Options parseOptions(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        auto needValue = [&](const char* name) -> std::string {
            if (++i >= argc) throw std::runtime_error(std::string(name) + " needs a value");
            return argv[i];
        };

        if (arg == "--help" || arg == "-h") {
            usage(std::cout);
            std::exit(0);
        } else if (arg == "--pd-code") {
            options.pdText = needValue("--pd-code");
        } else if (arg == "--pd-file") {
            options.pdText = readWholeFile(needValue("--pd-file"));
        } else if (arg == "--timeout") {
            options.timeoutSeconds = std::stoi(needValue("--timeout"));
            if (options.timeoutSeconds < 0) throw std::runtime_error("--timeout must be >= 0");
        } else if (arg == "--data-root") {
            options.dataRoot = needValue("--data-root");
        } else if (arg == "--print-invariants") {
            options.printInvariants = true;
        } else if (arg == "--verbose") {
            options.verbose = true;
        } else {
            throw std::runtime_error("unknown option: " + arg);
        }
    }

    if (options.pdText.empty()) {
        std::ostringstream buffer;
        buffer << std::cin.rdbuf();
        options.pdText = buffer.str();
    }
    if (options.pdText.empty()) throw std::runtime_error("no PD code was provided");
    return options;
}

std::vector<std::string> sortedUnique(std::vector<std::string> values) {
    std::sort(values.begin(), values.end());
    values.erase(std::unique(values.begin(), values.end()), values.end());
    return values;
}

std::vector<std::string> intersectNames(const std::vector<std::string>& a, const std::vector<std::string>& b) {
    std::vector<std::string> left = sortedUnique(a);
    std::vector<std::string> right = sortedUnique(b);
    std::vector<std::string> out;
    std::set_intersection(left.begin(), left.end(), right.begin(), right.end(), std::back_inserter(out));
    return out;
}

std::vector<std::string> mergeCandidates(const WorkerResult& khResult,
                                         const std::vector<std::string>& khNames,
                                         const WorkerResult& homResult,
                                         const std::vector<std::string>& homNames) {
    if (khResult.success && homResult.success && !khNames.empty() && !homNames.empty()) {
        return intersectNames(khNames, homNames);
    }
    std::vector<std::string> combined;
    if (khResult.success) combined.insert(combined.end(), khNames.begin(), khNames.end());
    if (homResult.success) combined.insert(combined.end(), homNames.begin(), homNames.end());
    return sortedUnique(combined);
}

void printWorkerStatus(const char* name, const WorkerResult& result) {
    std::cerr << name << ": ";
    if (result.success) {
        std::cerr << "ok";
    } else if (result.timedOut) {
        std::cerr << "timeout";
    } else if (result.interrupted) {
        std::cerr << "interrupted";
    } else {
        std::cerr << "failed(exit=" << result.exitCode << ")";
    }
    std::cerr << ", " << result.seconds << "s\n";
}

void printWorkerDetails(const char* name, const WorkerResult& result, bool printValue) {
    printWorkerStatus(name, result);
    if (result.success) {
        if (printValue) std::cerr << name << " result: " << result.output << "\n";
        return;
    }
    if (result.timedOut) {
        std::cerr << name << " detail: timed out after " << result.seconds << "s\n";
    } else if (result.interrupted) {
        std::cerr << name << " detail: interrupted\n";
    } else {
        std::cerr << name << " detail: failed with exit code " << result.exitCode << "\n";
    }
    if (!result.error.empty()) {
        std::cerr << name << " stderr:\n" << result.error << "\n";
    }
}

int workerMain(int argc, char** argv) {
    std::string worker;
    std::filesystem::path inputPath;
    std::filesystem::path outputPath;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        auto needValue = [&](const char* name) -> std::string {
            if (++i >= argc) throw std::runtime_error(std::string(name) + " needs a value");
            return argv[i];
        };
        if (arg == "--worker") worker = needValue("--worker");
        else if (arg == "--input") inputPath = needValue("--input");
        else if (arg == "--output") outputPath = needValue("--output");
    }

    if (worker.empty() || inputPath.empty() || outputPath.empty()) {
        throw std::runtime_error("bad worker invocation");
    }

    PDCode pd = parsePDCode(readWholeFile(inputPath));
    std::string value;
    if (worker == "khovanov") value = computeKhovanov(pd);
    else if (worker == "homfly") value = computeHomflyPT(pd);
    else throw std::runtime_error("unknown worker: " + worker);
    writeWholeFile(outputPath, value);
    return 0;
}

} // namespace

} // namespace hki

int main(int argc, char** argv) {
    try {
        for (int i = 1; i < argc; ++i) {
            if (std::string(argv[i]) == "--worker") return hki::workerMain(argc, argv);
        }

        hki::installInterruptHandlers();
        hki::Options options = hki::parseOptions(argc, argv);
        hki::PDCode pd = hki::parsePDCode(options.pdText);
        std::string canonicalPd = hki::formatPDCodeList(pd);
        std::filesystem::path executable = hki::currentExecutablePath(argv[0]);

        hki::WorkerResult khResult = hki::runWorkerProcess(executable, "khovanov", canonicalPd, options.timeoutSeconds);
        if (hki::interrupted() || khResult.interrupted) {
            std::cerr << "Interrupted; exiting cleanly.\n";
            return 130;
        }
        hki::WorkerResult homResult = hki::runWorkerProcess(executable, "homfly", canonicalPd, options.timeoutSeconds);
        if (hki::interrupted() || homResult.interrupted) {
            std::cerr << "Interrupted; exiting cleanly.\n";
            return 130;
        }

        if (options.verbose) {
            hki::printWorkerDetails("Khovanov", khResult, true);
            hki::printWorkerDetails("HOMFLY-PT", homResult, true);
        } else if (options.printInvariants) {
            std::cerr << "Khovanov result: " << (khResult.success ? khResult.output : "<failed>") << "\n";
            std::cerr << "HOMFLY-PT result: " << (homResult.success ? homResult.output : "<failed>") << "\n";
        }

        if (!khResult.success && !homResult.success) {
            std::cerr << "Both invariant computations failed or timed out.\n";
            return 2;
        }

        hki::DataPaths dataPaths = hki::findDataPaths(executable, options.dataRoot);
        hki::NameNormalizer normalizer(dataPaths.normalizerDir);
        hki::InvariantMap khDb = hki::loadInvariantMap(dataPaths.khovanovDb, normalizer);
        hki::InvariantMap homDb = hki::loadInvariantMap(dataPaths.homflyDb, normalizer);

        std::vector<std::string> khNames = khResult.success ? hki::lookupInvariant(khDb, khResult.output) : std::vector<std::string>{};
        std::vector<std::string> homNames = homResult.success ? hki::lookupInvariant(homDb, homResult.output) : std::vector<std::string>{};
        std::vector<std::string> candidates = hki::mergeCandidates(khResult, khNames, homResult, homNames);

        for (const std::string& name : candidates) std::cout << name << "\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }
}
