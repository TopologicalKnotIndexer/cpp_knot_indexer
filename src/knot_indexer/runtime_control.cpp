#include "runtime_control.h"

#include <atomic>
#include <csignal>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace hki {

namespace {
std::atomic_bool gInterrupted{false};

void signalHandler(int) {
    gInterrupted.store(true);
}

#ifdef _WIN32
BOOL WINAPI consoleHandler(DWORD type) {
    if (type == CTRL_C_EVENT || type == CTRL_BREAK_EVENT || type == CTRL_CLOSE_EVENT ||
        type == CTRL_LOGOFF_EVENT || type == CTRL_SHUTDOWN_EVENT) {
        gInterrupted.store(true);
        return TRUE;
    }
    return FALSE;
}
#endif

} // namespace

void installInterruptHandlers() {
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
#ifdef _WIN32
    SetConsoleCtrlHandler(consoleHandler, TRUE);
#endif
}

bool interrupted() {
    return gInterrupted.load();
}

void requestInterrupt() {
    gInterrupted.store(true);
}

} // namespace hki
