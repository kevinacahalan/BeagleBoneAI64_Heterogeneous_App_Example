#ifndef EXAMPLE_ERRNO_H
#define EXAMPLE_ERRNO_H

#include <errno.h>
#include <stdio.h>
#include <string.h>

typedef enum {
    EEXAMPLE_SUCCESS = 1000,
    EEXAMPLE_RPMSG,
    EEXAMPLE_TIMEOUT,
    EEXAMPLE_PROTOCOL,
    EEXAMPLE_ARGUMENT,
    EEXAMPLE_STATE,
} example_errno_t;

static inline const char *example_strerror(int errnum)
{
    switch (errnum) {
    case EEXAMPLE_SUCCESS:
        return "Success";
    case EEXAMPLE_RPMSG:
        return "RPMSG transport error";
    case EEXAMPLE_TIMEOUT:
        return "Operation timed out";
    case EEXAMPLE_PROTOCOL:
        return "Protocol validation failure";
    case EEXAMPLE_ARGUMENT:
        return "Invalid argument";
    case EEXAMPLE_STATE:
        return "Invalid runtime state";
    default:
        return strerror(errnum);
    }
}

static inline void example_perror(const char *prefix)
{
    fprintf(stderr, "%s: %s\n", prefix ? prefix : "example", example_strerror(errno));
}

#endif