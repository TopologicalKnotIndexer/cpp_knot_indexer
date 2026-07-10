#include "process_runner.h"

#include "runtime_control.h"

#include <chrono>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace hki {

namespace {

std::string readFile(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return "";
    std::ostringstream buffer;
    buffer << in.rdbuf();
    std::string value = buffer.str();
    while (!value.empty() && (value.back() == '\n' || value.back() == '\r')) value.pop_back();
    return value;
}

void writeFile(const std::filesystem::path& path, const std::string& value) {
    std::ofstream out(path, std::ios::binary);
    if (!out) throw std::runtime_error("cannot write temporary input file: " + path.string());
    out << value;
}

std::filesystem::path uniqueTempPath(const char* suffix) {
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
#ifdef _WIN32
    unsigned long pid = GetCurrentProcessId();
#else
    unsigned long pid = static_cast<unsigned long>(getpid());
#endif
    std::ostringstream name;
    name << "hki_" << pid << "_" << now << suffix;
    return std::filesystem::temp_directory_path() / name.str();
}

struct TempFiles {
    std::filesystem::path input;
    std::filesystem::path output;
    std::filesystem::path error;

    TempFiles() : input(uniqueTempPath(".in")), output(uniqueTempPath(".out")), error(uniqueTempPath(".err")) {}
    ~TempFiles() {
        std::error_code ec;
        std::filesystem::remove(input, ec);
        std::filesystem::remove(output, ec);
        std::filesystem::remove(error, ec);
    }
};

#ifdef _WIN32

std::wstring quoteWindowsArg(const std::wstring& arg) {
    std::wstring out = L"\"";
    unsigned backslashes = 0;
    for (wchar_t c : arg) {
        if (c == L'\\') {
            ++backslashes;
        } else if (c == L'"') {
            out.append(backslashes * 2 + 1, L'\\');
            out.push_back(c);
            backslashes = 0;
        } else {
            out.append(backslashes, L'\\');
            backslashes = 0;
            out.push_back(c);
        }
    }
    out.append(backslashes * 2, L'\\');
    out.push_back(L'"');
    return out;
}

WorkerResult runProcessPlatform(const std::filesystem::path& executable,
                                const std::string& workerName,
                                const std::filesystem::path& input,
                                const std::filesystem::path& output,
                                const std::filesystem::path& error,
                                int timeoutSeconds) {
    WorkerResult result;
    auto started = std::chrono::steady_clock::now();

    std::wstring command = quoteWindowsArg(executable.wstring()) +
        L" --worker " + quoteWindowsArg(std::wstring(workerName.begin(), workerName.end())) +
        L" --input " + quoteWindowsArg(input.wstring()) +
        L" --output " + quoteWindowsArg(output.wstring());

    STARTUPINFOW si{};
    PROCESS_INFORMATION pi{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    si.wShowWindow = SW_HIDE;

    HANDLE stderrHandle = CreateFileW(error.wstring().c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                                      CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, nullptr);
    if (stderrHandle == INVALID_HANDLE_VALUE) throw std::runtime_error("cannot create temporary stderr file");
    SetHandleInformation(stderrHandle, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    si.hStdError = stderrHandle;

    std::vector<wchar_t> mutableCommand(command.begin(), command.end());
    mutableCommand.push_back(L'\0');

    BOOL ok = CreateProcessW(executable.wstring().c_str(), mutableCommand.data(), nullptr, nullptr, TRUE,
                             CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
    if (!ok) {
        CloseHandle(stderrHandle);
        throw std::runtime_error("CreateProcessW failed for worker");
    }

    DWORD exitCode = STILL_ACTIVE;
    const auto timeout = timeoutSeconds > 0 ? std::chrono::seconds(timeoutSeconds) : std::chrono::seconds::max();
    while (true) {
        DWORD wait = WaitForSingleObject(pi.hProcess, 100);
        if (wait == WAIT_OBJECT_0) {
            GetExitCodeProcess(pi.hProcess, &exitCode);
            break;
        }
        if (interrupted()) {
            TerminateProcess(pi.hProcess, 130);
            result.interrupted = true;
            exitCode = 130;
            break;
        }
        if (std::chrono::steady_clock::now() - started >= timeout) {
            TerminateProcess(pi.hProcess, 124);
            result.timedOut = true;
            exitCode = 124;
            break;
        }
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    CloseHandle(stderrHandle);

    result.exitCode = static_cast<int>(exitCode);
    result.seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
    result.success = !result.timedOut && !result.interrupted && result.exitCode == 0;
    if (result.success) result.output = readFile(output);
    result.error = readFile(error);
    return result;
}

#else

WorkerResult runProcessPlatform(const std::filesystem::path& executable,
                                const std::string& workerName,
                                const std::filesystem::path& input,
                                const std::filesystem::path& output,
                                const std::filesystem::path& error,
                                int timeoutSeconds) {
    WorkerResult result;
    auto started = std::chrono::steady_clock::now();

    pid_t pid = fork();
    if (pid < 0) throw std::runtime_error("fork failed for worker");
    if (pid == 0) {
        std::string exe = executable.string();
        std::string in = input.string();
        std::string out = output.string();
        std::string err = error.string();
        FILE* errFile = std::fopen(err.c_str(), "wb");
        if (errFile) {
            dup2(fileno(errFile), STDERR_FILENO);
            std::fclose(errFile);
        }
        execl(exe.c_str(), exe.c_str(), "--worker", workerName.c_str(), "--input", in.c_str(), "--output", out.c_str(), static_cast<char*>(nullptr));
        _exit(127);
    }

    int status = 0;
    const auto timeout = timeoutSeconds > 0 ? std::chrono::seconds(timeoutSeconds) : std::chrono::seconds::max();
    while (true) {
        pid_t done = waitpid(pid, &status, WNOHANG);
        if (done == pid) break;
        if (done < 0) {
            result.exitCode = -1;
            break;
        }
        if (interrupted()) {
            kill(pid, SIGTERM);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            kill(pid, SIGKILL);
            waitpid(pid, &status, 0);
            result.interrupted = true;
            break;
        }
        if (std::chrono::steady_clock::now() - started >= timeout) {
            kill(pid, SIGTERM);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            kill(pid, SIGKILL);
            waitpid(pid, &status, 0);
            result.timedOut = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (WIFEXITED(status)) result.exitCode = WEXITSTATUS(status);
    else if (WIFSIGNALED(status)) result.exitCode = 128 + WTERMSIG(status);

    result.seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
    result.success = !result.timedOut && !result.interrupted && result.exitCode == 0;
    if (result.success) result.output = readFile(output);
    result.error = readFile(error);
    return result;
}

#endif

} // namespace

WorkerResult runWorkerProcess(const std::filesystem::path& executable,
                              const std::string& workerName,
                              const std::string& input,
                              int timeoutSeconds) {
    TempFiles temp;
    writeFile(temp.input, input);
    return runProcessPlatform(executable, workerName, temp.input, temp.output, temp.error, timeoutSeconds);
}

} // namespace hki
