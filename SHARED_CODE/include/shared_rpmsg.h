#define RPMSG_CHAR_DEVICE_NAME "rpmsg_chrdev" // No idea what is up with this magic string
#define RPMSG_CHAR_ENDPOINT 14U // or this magic number

typedef enum
{
    UNKNOWN = 0,
    FUNCTION_A = 1,
    FUNCTION_B = 2,
} FUNCTION_TAG;

typedef struct {
    FUNCTION_TAG tag;
    union {
        struct {
            int a;
            int b;
        } function_a;  // Named member for FUNCTION_A arguments
        struct {
            float a;
            float b;
            float c;
        } function_b;  // Named member for FUNCTION_B arguments
    };
} request_tagged_union_t;