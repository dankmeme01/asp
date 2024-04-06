#include <asp/Log.hpp>

#include <iostream>

namespace asp {

static std::function<void(LogLevel, const std::string_view)> LOG_FUNC = [](LogLevel level, auto message) {
    std::cerr << "[asp] " << message << std::endl;
};

void setLogFunction(std::function<void(LogLevel, const std::string_view)>&& f) {
    LOG_FUNC = std::move(f);
}

void doLog(LogLevel level, const std::string_view message) {
    LOG_FUNC(level, message);
}

}