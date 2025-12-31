#pragma once

#if defined(__clang__) && !defined(ATTRIBUTE)
    // Clang-only: supports annotate
    #define ATTRIBUTE(...) __attribute__((annotate(#__VA_ARGS__)))
#elif !defined(ATTRIBUTE)
    // GCC, MSVC, others: do nothing
    #define ATTRIBUTE(...) [[maybe_unused]]
#endif

#define DECLARE_DESCRIPTOR(Type)           \
public:                                              \
    static ComponentDescriptor descriptor;           \
    const ComponentDescriptor* GetDescriptor() const override { \
        return &Type::descriptor;                    \
    }
