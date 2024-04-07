#include <asp/async/Future.hpp>
#include <asp/Log.hpp>

namespace asp::async {

namespace detail {
    void futureFail(const std::exception& e) {
        asp::log(asp::LogLevel::Error, std::string("Future threw: ") + e.what());
    }
}

}