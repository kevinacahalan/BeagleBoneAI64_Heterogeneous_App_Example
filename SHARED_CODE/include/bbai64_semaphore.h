// WARNING!!!! This code is not truly atomic and is susceptible  to RACE CONDITIONS!!!
// WE NEED TO MAKE USE A MEMORY BARRIERS OR HARDWARE SEMAPHORES!!!!

// Without true atomic operations we have to keep Peterson's Algorithm in mind...
// BUT BE WARNED!! Peterson's Algorithm breaks with memory reordering!
// https://en.wikipedia.org/wiki/Peterson%27s_algorithm



#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include "bbai64_atomic.h"

typedef struct {
    volatile int value;
} semaphore_t;

/**
 * Initializes the semaphore with a given value.
 * 
 * @param sem Pointer to the semaphore.
 * @param value Initial value of the semaphore.
 */
static inline void sem_init(volatile semaphore_t *sem, int value) {
    sem->value = value;
}

/**
 * Wait (decrement) operation on the semaphore.
 * Blocks until the semaphore value is greater than zero, then decrements it.
 * 
 * @param sem Pointer to the semaphore.
 */
static inline void sem_wait(volatile semaphore_t *sem) {
    while (1) {
        int old_value = sem->value;
        if (old_value > 0 && not_actually_atomic_compare_exchange(&sem->value, old_value, old_value - 1) == 0) {
            break;
        }
    }
}

/**
 * Post (increment) operation on the semaphore.
 * Increments the semaphore value, potentially waking up a waiting thread.
 * 
 * @param sem Pointer to the semaphore.
 */
static inline void sem_post(volatile semaphore_t *sem) {
    while (1) {
        int old_value = sem->value;
        if (not_actually_atomic_compare_exchange(&sem->value, old_value, old_value + 1) == 0) {
            break;
        }
    }
}

/**
 * Try-wait (non-blocking decrement) operation on the semaphore.
 * Attempts to decrement the semaphore value without blocking.
 * 
 * @param sem Pointer to the semaphore.
 * @return 0 on success, -1 if the semaphore value is zero.
 */
static inline int sem_trywait(volatile semaphore_t *sem) {
    int old_value = sem->value;
    if (old_value > 0 && not_actually_atomic_compare_exchange(&sem->value, old_value, old_value - 1) == 0) {
        return 0; // Success
    }
    return -1; // Failure
}

/**
 * Get the current value of the semaphore.
 * 
 * @param sem Pointer to the semaphore.
 * @return The current value of the semaphore.
 */
static inline int sem_getvalue(volatile semaphore_t *sem) {
    return sem->value;
}

#endif // SEMAPHORE_H
