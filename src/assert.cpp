#include <asp/config.hpp>
#include <stdexcept>

void asp::detail::assertionFail(const char* message) {
    throw std::runtime_error(std::string("asp assertion failed: ") + message);
}