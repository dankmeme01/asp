#pragma once

#include "config.hpp"
#include <functional>
#include <string_view>

namespace asp {
    enum class LogLevel {
        Trace, Debug, Info, Warn, Error
    };

    void setLogFunction(std::function<void(LogLevel, const std::string_view)>&& f);
    void log(LogLevel level, const std::string_view message);
}