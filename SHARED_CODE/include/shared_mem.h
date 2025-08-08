#ifndef SHARED_MEM_H
#define SHARED_MEM_H

#include <stdint.h>
#include <stddef.h>
#include "bbai64_semaphore.h"

// Size definitions
#define TOTAL_SIZE_BYTES (16 * 1024 * 1024) // Total size: 16MB
#define STATUS_SIZE_BYTES sizeof(SharedMemStatus) // Size of SharedMemStatus struct

// Calculate remaining size for the data array in terms of uint32_t elements
#define DATA_SIZE ((TOTAL_SIZE_BYTES - STATUS_SIZE_BYTES) / sizeof(uint32_t))


// Define the struct to hold both status enums
typedef struct {
    semaphore_t sem_producer;
    semaphore_t sem_consumer;
} SharedMemStatus;

// Define the struct for the 16MB shared memory region
typedef struct {
    volatile SharedMemStatus status; // Combined status flags at the beginning
    volatile uint32_t data[DATA_SIZE]; // Remaining space for data
} SharedMemoryRegion;



// NEEDS C11
_Static_assert(sizeof(SharedMemoryRegion) == TOTAL_SIZE_BYTES, "SharedMemoryRegion does not match the expected 16MB size");

#define SHARED_MEM_SIZE 0x1000000

#if R5
extern volatile int __shared_mem_base[]; // declared in linker script
extern volatile SharedMemoryRegion* sharedMem;

#else // LINUX

extern void update_shared_globals(void);
extern size_t strlen_aligned16(const char *str);
extern int memcpy_aligned16(void *dest, const void *src, size_t size);
extern int memcpy_aligned16_volatile(volatile void *dest, const volatile void *src, size_t size);
extern unsigned int setup_shared_buffer(void);
extern void deinit_shared_mem(void);


extern volatile SharedMemoryRegion* sharedMem;
#endif


#endif // SHARED_MEM_H
