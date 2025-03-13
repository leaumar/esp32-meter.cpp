#pragma once

#include <cstddef>
#include <type_traits>

// chatgpt
// TODO try raising language level

namespace std
{
    // Overload for raw arrays
    template <typename T, std::size_t N>
    constexpr std::size_t size(const T (&)[N]) noexcept
    {
        return N;
    }

    // Overload for containers with .size()
    template <typename C>
    constexpr auto size(const C &c) -> decltype(c.size())
    {
        return c.size();
    }
}
