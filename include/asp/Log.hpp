#pragma once

#include "config.hpp"
#include <functional>
#include <string_view>
#include <format>

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

    template <class... Args>
    inline void log(LogLevel level, std::format_string<Args...> fmt, Args&&... args) {
    #ifndef ASP_DEBUG
        // disable trace logs in release
        if (level == LogLevel::Trace) return;
    #endif
        doLog(level, std::format(fmt, std::forward<Args>(args)...));
    }

    inline void trace(const std::string_view message) {
        log(LogLevel::Trace, message);
    }

    template <class... Args>
    inline void trace(std::format_string<Args...> fmt, Args&&... args) {
        log(LogLevel::Trace, fmt, std::forward<Args>(args)...);
    }
}