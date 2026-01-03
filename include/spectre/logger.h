#pragma once

#include <mutex>
#include <iostream>

namespace spectre {
    // Global mutex for console output
    inline std::mutex console_mutex;

    // Helper to lock and print
    struct ScopedLogger {
        std::lock_guard<std::mutex> lock;
        ScopedLogger() : lock(console_mutex) {}
    };
}