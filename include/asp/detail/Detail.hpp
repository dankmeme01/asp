#pragma once

#include <bit>

#if defined(__cpp_lib_bit_cast) && __cpp_lib_bit_cast >= 201806L
namespace asp::detail {
    template <typename To, typename From>
    static inline constexpr To bit_cast(From value) noexcept {
        return std::bit_cast<To, From>(value);
    }
}
#else
#include <type_traits>
#include <cstring>
namespace asp::detail {
    template <typename To, typename From>
    static inline
    ::std::enable_if_t<sizeof(To) == sizeof(From) && ::std::is_trivially_copyable_v<From> && std::is_trivially_copyable_v<To>, To>
    bit_cast(From value) noexcept {
        static_assert(std::is_trivially_constructible_v<To>, "destination template argument for bit_cast must be trivially constructible");

        To dest;
        ::std::memcpy(&dest, &value, sizeof(To));
        return dest;
    }
}
#endif
