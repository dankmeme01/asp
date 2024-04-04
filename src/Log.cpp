#include <asp/Log.hpp>

#include <iostream>

namespace asp {

static std::function<void(LogLevel, const std::string_view)> LOG_FUNC = [](LogLevel level, auto message) {
    std::cerr << "[asp] " << message << std::endl;
};

void setLogFunction(std::function<void(LogLevel, const std::string_view)>&& f) {
    LOG_FUNC = std::move(f);
}

void log(LogLevel level, const std::string_view message) {
#ifndef ASP_DEBUG
    // disable trace logs in release
    if (level == LogLevel::Trace) return;
#endif
    LOG_FUNC(level, message);
}

}