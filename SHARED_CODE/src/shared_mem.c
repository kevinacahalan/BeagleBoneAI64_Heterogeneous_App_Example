#if R5

#include <ai64/shared_mem.h>
#include <ai64/bbai64_gpio.h>
#include <ti/csl/csl_gpio.h>

volatile SharedMemoryRegion* sharedMem = (volatile SharedMemoryRegion*) __shared_mem_base;

#else // Linux


#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <time.h>

#include "../include/shared_mem.h"


#define SHARED_MEM_PHYSICAL_BASE 0x90000000 // from device tree overlay and from linkerscript
#define SHARED_MEM_SIZE 0x1000000 // from device tree overlay and from linkerscript

volatile SharedMemoryRegion* sharedMem = NULL;

size_t strlen_aligned16(const char *str) {
    // Ensure str is 16-bit aligned; if not, handle byte-by-byte until alignment
    uintptr_t addr = (uintptr_t)str;
    while (addr % sizeof(uint16_t) != 0) {
        if (*str == '\0') {
            return str - (const char *)addr;
        }
        str++;
        addr = (uintptr_t)str;
    }

    // Now str is aligned to 16 bits, we can read 16 bits at a time
    const uint16_t *aligned_str = (const uint16_t *)str;
    uint16_t current_value;

    while (1) {
        current_value = *aligned_str;

        // Check if current 16 bits contain a null byte
        if ((current_value & 0x00FF) == 0) {
            return (const char *)aligned_str - (const char *)addr;
        }
        if ((current_value & 0xFF00) == 0) {
            return ((const char *)aligned_str - (const char *)addr) + 1;
        }

        aligned_str++;
    }
}

#define UNALIGNED_ERROR EINVAL
#define ODD_SIZE_ERROR EINVAL

int memcpy_aligned16(void *dest, const void *src, size_t size) {
    // Check for 16-bit alignment of both source and destination
    if (((uintptr_t)dest % sizeof(uint16_t) != 0) || ((uintptr_t)src % sizeof(uint16_t) != 0)) {
        errno = UNALIGNED_ERROR;
        return -1;
    }

    // Check if n is an odd number
    if (size % 2 != 0) {
        errno = ODD_SIZE_ERROR;
        return -1;
    }

    uint16_t *dest16 = (uint16_t *)dest;
    const uint16_t *src16 = (const uint16_t *)src;

    // Perform the copy in 16-bit chunks
    for (size_t i = 0; i < size / 2; ++i) {
        dest16[i] = src16[i];
    }

    return 0; // Success
}

int memcpy_aligned16_volatile(volatile void *dest, const volatile void *src, size_t size) {
    // Check for 16-bit alignment of both source and destination
    if (((uintptr_t)dest % sizeof(uint16_t) != 0) || ((uintptr_t)src % sizeof(uint16_t) != 0)) {
        errno = UNALIGNED_ERROR;
        return -1;
    }

    // Check if n is an odd number
    if (size % 2 != 0) {
        errno = ODD_SIZE_ERROR;
        return -1;
    }

    // Cast both source and destination to volatile pointers to respect the volatile semantics
    volatile uint16_t *dest16 = (volatile uint16_t *)dest;
    const volatile uint16_t *src16 = (const volatile uint16_t *)src;

    // Perform the copy in 16-bit chunks, ensuring volatile access semantics are respected for both read and write
    for (size_t i = 0; i < size / 2; ++i) {
        // Use volatile read and write to copy data
        dest16[i] = src16[i];
    }

    return 0; // Success
}

static size_t full_pages(const size_t size)
{
    size_t page = sysconf(_SC_PAGESIZE);
    if (size < page)
        return page;
    else
    if (size % page)
        return size + page - (size % page);
    else
        return size;
}


int shared_mem_fd = -1;

unsigned int setup_shared_buffer(void){
    size_t shared_size = full_pages(SHARED_MEM_SIZE);
    if (shared_size != SHARED_MEM_SIZE) {
        printf("Error: Shared memory not paged aligned, requested size: %d, smaller aligned size: %ld\n", SHARED_MEM_SIZE, shared_size);
    }
    shared_mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (shared_mem_fd < 0) {
        perror("open");
        return 1;
    }

    // Probably not needed
    ftruncate(shared_mem_fd, SHARED_MEM_SIZE);

    // Map shared memory
    void *shared_mem_mapping = mmap(NULL, SHARED_MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shared_mem_fd, SHARED_MEM_PHYSICAL_BASE);
    if (shared_mem_mapping == MAP_FAILED) {
        perror("mmap");
        close(shared_mem_fd);
        return 1;
    }

    printf("Setup memory at physical address %x setup successfully\n", SHARED_MEM_PHYSICAL_BASE);
    fflush(stdout);

    // Assign to globl pointer
    sharedMem = shared_mem_mapping;

    return 0;
}

void deinit_shared_mem(void) {
    if (munmap((void*)sharedMem, SHARED_MEM_SIZE) == -1) {
        perror("munmap");
    }
    close(shared_mem_fd);
}

#endif