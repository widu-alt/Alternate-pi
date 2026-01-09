#pragma once

#include <mutex>
#include <iostream>

namespace spectre {
    // Global mutex for console output
    inline std::mutex console_mutex;

    // Helper: When you create this variable, it locks the console.
    // When it goes out of scope (end of function/block), it unlocks
    struct ScopedLogger {
        std::lock_guard<std::mutex> lock;
        ScopedLogger() : lock(console_mutex) {}
    };
}