#include <asp/config.hpp>
#include <stdexcept>
#include <string>

void asp::detail::assertionFail(const char* message) {
    throw std::runtime_error(std::string("asp assertion failed: ") + message);
}