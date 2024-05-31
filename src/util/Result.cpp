#include <asp/util/Result.hpp>

#include <stdexcept>

namespace asp::util {
    class ResultException : public std::runtime_error {
    public:
        ResultException(const std::string& message) : runtime_error(message) {}
    };

    void throwResultError(const std::string& message) {
        throw ResultException(message);
    }
}