#pragma once

#if (defined(__clang__) || defined(__GNUC__)) && !defined(ATTRIBUTE)
    // Variadic macro for multiple attributes
    #define ATTRIBUTE(...) __attribute__((annotate(#__VA_ARGS__)))
#elif !defined(ATTRIBUTE)
    // fallback for MSVC or unknown
    #define ATTRIBUTE(...) [[maybe_unused]]
#endif
