#ifndef BBAI64_ATOMIC_H
#define BBAI64_ATOMIC_H
#include <assert.h>

#if defined(__arm__) || defined(__aarch64__)  // This should cover both ARMv7 and ARMv8 architectures

#ifdef R5

// NOT ACTUALLY ATOMIC
// maybe should add use of HwiP_disable() and HwiP_restore()
static inline int not_actually_atomic_compare_exchange(volatile int *ptr, int old_value, int new_value) {
    // Ensure ptr is 4-byte aligned
    assert(((uintptr_t)ptr & 0x3) == 0);

    int current_value = *ptr;
    if (current_value == old_value) {
        *ptr = new_value;
        return 0; // Success
    }
    return 1; // Failure
}

#else

// NOT ACTUALLY ATOMIC
static inline int not_actually_atomic_compare_exchange(volatile int *ptr, int old_value, int new_value) {
    // Ensure ptr is 4-byte aligned
    assert(((uintptr_t)ptr & 0x3) == 0);

    int current_value = *ptr;
    if (current_value == old_value) {
        *ptr = new_value;
        return 0; // Success
    }
    return 1; // Failure
}

#endif

#else // Not ARM architecture

// Fallback for other architectures
#include <stdatomic.h>

static inline int not_actually_atomic_compare_exchange(volatile int *ptr, int old_value, int new_value) {
    int expected = old_value;
    return atomic_compare_exchange_strong((atomic_int *)ptr, &expected, new_value);
}

#endif // __arm__ or __aarch64__
#endif // BBAI64_ATOMIC_H
