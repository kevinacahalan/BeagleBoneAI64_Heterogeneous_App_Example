#define RPMSG_CHAR_DEVICE_NAME "rpmsg_chrdev" // No idea what is up with this magic string
#define RPMSG_CHAR_ENDPOINT 14U // or this magic number


typedef enum {
    MESSAGE_UNKNOWN = 0,
    MESSAGE_REQUEST = 1,
    MESSAGE_RESPONSE = 2,
} MESSAGE_TAG;

typedef enum {
    FUNCTION_UNKNOWN = 0,
    FUNCTION_A = 1,    // Takes two ints, returns an int (on R5)
    FUNCTION_B = 2,    // Takes three floats, returns a float (on R5)

    // Linux functions
    FUNCTION_X = 3,    // Takes an int, returns a float (on Linux)
    FUNCTION_Y = 4,    // Takes a float, returns an int (on Linux)
} FUNCTION_TAG;

typedef struct {
    FUNCTION_TAG function_tag;
    union {
        // R5 functions
        struct {
            int a;
            int b;
        } function_a;
        struct {
            float a;
            float b;
            float c;
        } function_b;

        // linux functions
        struct {
            int param;
        } function_x;
        struct {
            float param;
        } function_y;
    } params;
} request_data_t;

typedef struct {
    FUNCTION_TAG function_tag;
    union {
        int result_function_a;
        float result_function_b;
        float result_function_x;
        int result_function_y;
    } result;
} response_data_t;

// typedef struct __attribute__((packed)) { // packing caused sized to not match between cores...
typedef struct {
    MESSAGE_TAG tag;
    uint32_t request_id;
    union {
        request_data_t request;
        response_data_t response;
    } data;
} MESSAGE;

