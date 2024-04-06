#pragma once

#include "config.hpp"
#include <functional>
#include <string_view>

namespace asp {
    enum class LogLevel {
        Trace, Debug, Info, Warn, Error
    };

    void setLogFunction(std::function<void(LogLevel, const std::string_view)>&& f);
    void doLog(LogLevel level, const std::string_view message);

    inline void log(LogLevel level, const std::string_view message) {
    #ifndef ASP_DEBUG
        // disable trace logs in release
        if (level == LogLevel::Trace) return;
    #endif
        doLog(level, message);
    }

    inline void trace(const std::string_view message) {
        log(LogLevel::Trace, message);
    }
}