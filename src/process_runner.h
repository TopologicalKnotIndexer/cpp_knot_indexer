#pragma once

#include <filesystem>
#include <string>

namespace hki {

struct WorkerResult {
    bool success = false;
    bool timedOut = false;
    bool interrupted = false;
    int exitCode = -1;
    double seconds = 0.0;
    std::string output;
    std::string error;
};

WorkerResult runWorkerProcess(const std::filesystem::path& executable,
                              const std::string& workerName,
                              const std::string& input,
                              int timeoutSeconds);

} // namespace hki
