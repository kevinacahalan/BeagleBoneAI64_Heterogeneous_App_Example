#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "../../../SHARED_CODE/include/dbg.h"
#include "../../../SHARED_CODE/include/example_errno.h"
#include "../../../SHARED_CODE/include/random_utils.h"
#include <rpmsg_char_example.h>
#include "../../../SHARED_CODE/include/shared_rpmsg.h"
#include <ti_rpmsg_char.h>

#define TIMEOUT_DEFAULT_MS 5000U
#define RECONNECT_BACKOFF_MS 2000U
#define IDLE_POLL_DELAY_MS 10U
#define MAX_PENDING_MESSAGES_PER_CYCLE 8U
#define REMOTEPROC_RPMSG_GRACE_MS 5000U

static rpmsg_char_dev_t *rcdev = NULL;
static int rproc_id = R5F_MAIN0_0;
static unsigned int local_endpt = RPMSG_ADDR_ANY;
static unsigned int remote_endpt = RPMSG_CHAR_ENDPOINT;
static char *dev_name = RPMSG_CHAR_DEVICE_NAME;

static bool rpmsg_initialized = false;
static bool rpmsg_connected = false;
static bool rpmsg_connected_once = false;
static bool cleanup_registered = false;
static uint64_t next_reconnect_attempt_ms = 0;
static uint32_t req_id_counter = 1;
static int r5_remoteproc_instance = -2;
static bool r5_remoteproc_was_running = false;
static uint64_t r5_remoteproc_running_since_ms = 0;

static const char *const r5_remoteproc_names[] = {
    "j7-main-r5f0_0",
    "5c00000.r5f",
};

static inline uint32_t get_next_req_id(void)
{
    return req_id_counter++;
}

static uint64_t get_monotonic_ms(void)
{
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0;
    }

    return ((uint64_t)ts.tv_sec * 1000ULL) + ((uint64_t)ts.tv_nsec / 1000000ULL);
}

static void sleep_ms(unsigned int milliseconds)
{
    struct timespec req;

    req.tv_sec = milliseconds / 1000U;
    req.tv_nsec = (long)(milliseconds % 1000U) * 1000000L;
    nanosleep(&req, NULL);
}

static void trim_newline(char *text)
{
    size_t length;

    if (text == NULL) {
        return;
    }

    length = strlen(text);
    while (length > 0U && (text[length - 1U] == '\n' || text[length - 1U] == '\r')) {
        text[length - 1U] = '\0';
        length--;
    }
}

static int resolve_r5_remoteproc_instance(void)
{
    DIR *dir;
    struct dirent *entry;

    if (r5_remoteproc_instance >= -1) {
        return r5_remoteproc_instance;
    }

    dir = opendir("/sys/class/remoteproc");
    if (dir == NULL) {
        r5_remoteproc_instance = -1;
        return r5_remoteproc_instance;
    }

    while ((entry = readdir(dir)) != NULL) {
        char name_path[512];
        char remoteproc_name[128];
        FILE *name_file;
        int instance = -1;

        if (sscanf(entry->d_name, "remoteproc%d", &instance) != 1) {
            continue;
        }

        snprintf(name_path, sizeof(name_path), "/sys/class/remoteproc/%s/name", entry->d_name);
        name_file = fopen(name_path, "r");
        if (name_file == NULL) {
            continue;
        }

        if (fgets(remoteproc_name, sizeof(remoteproc_name), name_file) != NULL) {
            size_t index;

            trim_newline(remoteproc_name);

            for (index = 0; index < (sizeof(r5_remoteproc_names) / sizeof(r5_remoteproc_names[0])); index++) {
                if (strcmp(remoteproc_name, r5_remoteproc_names[index]) == 0) {
                    fclose(name_file);
                    closedir(dir);
                    r5_remoteproc_instance = instance;
                    return r5_remoteproc_instance;
                }
            }
        }

        fclose(name_file);
    }

    closedir(dir);
    r5_remoteproc_instance = -1;
    return r5_remoteproc_instance;
}

static bool is_r5_remoteproc_running(void)
{
    char state_path[256];
    char state_text[64];
    FILE *state_file;
    int instance;

    instance = resolve_r5_remoteproc_instance();
    if (instance < 0) {
        return true;
    }

    snprintf(state_path, sizeof(state_path), "/sys/class/remoteproc/remoteproc%d/state", instance);
    state_file = fopen(state_path, "r");
    if (state_file == NULL) {
        return true;
    }

    if (fgets(state_text, sizeof(state_text), state_file) == NULL) {
        fclose(state_file);
        return true;
    }

    fclose(state_file);
    trim_newline(state_text);
    return strcmp(state_text, "running") == 0;
}

static bool is_r5_remoteproc_ready_for_rpmsg(uint64_t now_ms)
{
    bool running;

    running = is_r5_remoteproc_running();
    if (!running) {
        r5_remoteproc_was_running = false;
        r5_remoteproc_running_since_ms = 0;
        return false;
    }

    if (!r5_remoteproc_was_running) {
        r5_remoteproc_was_running = true;
        r5_remoteproc_running_since_ms = now_ms;
    }

    if ((now_ms - r5_remoteproc_running_since_ms) < REMOTEPROC_RPMSG_GRACE_MS) {
        return false;
    }

    return true;
}

static bool is_transport_error(int err)
{
    return err == EPIPE || err == ENODEV || err == ECONNRESET || err == EBADF || err == EIO || err == ENXIO;
}

static int get_rpmsg_fd(void)
{
    if (!rpmsg_connected || rcdev == NULL || rcdev->fd < 0) {
        errno = EEXAMPLE_STATE;
        return -1;
    }

    return rcdev->fd;
}

static int rpmsg_cleanup(void)
{
    int ret = 0;

    if (rcdev != NULL) {
        ret = rpmsg_char_close(rcdev);
        rcdev = NULL;
    }

    rpmsg_connected = false;
    if (rpmsg_initialized) {
        rpmsg_char_exit();
        rpmsg_initialized = false;
    }

    next_reconnect_attempt_ms = 0;
    return ret;
}

static void rpmsg_cleanup_atexit(void)
{
    if (rpmsg_cleanup() != 0) {
        log_warn("RPMSG cleanup reported an error during shutdown");
    }
}

static void rpmsg_mark_disconnected(const char *reason)
{
    if (rpmsg_connected) {
        log_warn("RPMSG disconnected (%s)", reason != NULL ? reason : "unknown");
    }

    rpmsg_connected = false;
    if (rcdev != NULL) {
        if (rcdev->fd >= 0) {
            close(rcdev->fd);
        }
        rcdev = NULL;
    }

    next_reconnect_attempt_ms = get_monotonic_ms() + RECONNECT_BACKOFF_MS;
}

static int rpmsg_try_connect(void)
{
    char eptdev_name[64];
    int flags;
    ssize_t bytes_written;
    MESSAGE ping = {0};
    uint64_t now_ms;
    bool was_connected_before = rpmsg_connected_once;

    if (rpmsg_connected && rcdev != NULL) {
        return 0;
    }

    now_ms = get_monotonic_ms();
    if (now_ms < next_reconnect_attempt_ms) {
        return -1;
    }

    if (!is_r5_remoteproc_running()) {
        errno = EEXAMPLE_RPMSG;
        log_info("R5 remoteproc is not running; next RPMSG probe in %u ms", RECONNECT_BACKOFF_MS);
        next_reconnect_attempt_ms = now_ms + RECONNECT_BACKOFF_MS;
        return -1;
    }

    if (!is_r5_remoteproc_ready_for_rpmsg(now_ms)) {
        errno = EEXAMPLE_RPMSG;
        log_info("R5 remoteproc is running; waiting for RPMSG channel startup, next probe in %u ms", RECONNECT_BACKOFF_MS);
        next_reconnect_attempt_ms = now_ms + RECONNECT_BACKOFF_MS;
        return -1;
    }

    if (!rpmsg_initialized) {
        int init_ret = rpmsg_char_init(NULL);
        if (init_ret != 0) {
            errno = EEXAMPLE_RPMSG;
            log_warn("RPMSG init unavailable (ret=%d); next connect attempt in %u ms", init_ret, RECONNECT_BACKOFF_MS);
            next_reconnect_attempt_ms = now_ms + RECONNECT_BACKOFF_MS;
            return -1;
        }
        rpmsg_initialized = true;
    }

    snprintf(eptdev_name, sizeof(eptdev_name), "eptdev_name_rpmsg%d-%d", rproc_id, getpid());
    rcdev = rpmsg_char_open(rproc_id, dev_name, local_endpt, remote_endpt, eptdev_name, 0);
    if (rcdev == NULL) {
        errno = EEXAMPLE_RPMSG;
        log_warn("RPMSG endpoint not ready; next connect attempt in %u ms", RECONNECT_BACKOFF_MS);
        next_reconnect_attempt_ms = now_ms + RECONNECT_BACKOFF_MS;
        return -1;
    }

    flags = fcntl(rcdev->fd, F_GETFL, 0);
    if (flags < 0 || fcntl(rcdev->fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        example_perror("Linux: Failed to configure RPMSG endpoint");
        rpmsg_char_close(rcdev);
        rcdev = NULL;
        next_reconnect_attempt_ms = now_ms + RECONNECT_BACKOFF_MS;
        return -1;
    }

    ping.tag = MESSAGE_PING;
    bytes_written = write(rcdev->fd, &ping, sizeof(ping));
    if (bytes_written != (ssize_t)sizeof(ping)) {
        if (bytes_written < 0) {
            example_perror("Linux: Failed to send RPMSG ping");
        } else {
            errno = EEXAMPLE_RPMSG;
            log_warn("Short write while sending RPMSG ping (%zd/%zu)", bytes_written, sizeof(ping));
        }
        rpmsg_char_close(rcdev);
        rcdev = NULL;
        next_reconnect_attempt_ms = now_ms + RECONNECT_BACKOFF_MS;
        return -1;
    }

    rpmsg_connected = true;
    rpmsg_connected_once = true;
    next_reconnect_attempt_ms = 0;
    log_info("RPMSG %s (fd=%d, port=%d)", was_connected_before ? "reconnected" : "connected", rcdev->fd, rcdev->endpt);
    return 0;
}

static int send_msg(int fd, const void *msg, size_t len)
{
    ssize_t ret;

    if (fd < 0) {
        errno = EEXAMPLE_STATE;
        return -1;
    }

    ret = write(fd, msg, len);
    if (ret < 0) {
        if (is_transport_error(errno)) {
            rpmsg_mark_disconnected("write");
            errno = EEXAMPLE_RPMSG;
            return -1;
        }
        example_perror("Linux: Unable to write to RPMSG endpoint");
        return -1;
    }

    if ((size_t)ret != len) {
        errno = EEXAMPLE_RPMSG;
        log_warn("Short write on RPMSG endpoint (%zd/%zu)", ret, len);
        return -1;
    }

    return 0;
}

static int recv_msg(int fd, void *buf, size_t len, int *recv_len)
{
    ssize_t ret;

    if (recv_len == NULL) {
        errno = EEXAMPLE_ARGUMENT;
        return -1;
    }
    *recv_len = 0;

    if (fd < 0) {
        errno = EEXAMPLE_STATE;
        return -1;
    }

    ret = read(fd, buf, len);
    if (ret < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;
        }
        if (is_transport_error(errno)) {
            rpmsg_mark_disconnected("read");
            errno = EEXAMPLE_RPMSG;
            return -1;
        }
        example_perror("Linux: Unable to read from RPMSG endpoint");
        return -1;
    }

    *recv_len = (int)ret;
    return 0;
}

static float linux_function_x(int param)
{
    return param * 1.5f;
}

static int linux_function_y(float param)
{
    return (int)(param * 2.0f);
}

static int handle_command_linux(const MESSAGE *cmd_msg)
{
    log_info("Received non-blocking command id=%d tag=%s", cmd_msg->request_id, function_tag_to_string(cmd_msg->data.request.function_tag));
    return 0;
}

static int handle_request_linux(const MESSAGE *req_msg)
{
    MESSAGE resp = {0};
    const request_data_t *req = &req_msg->data.request;

    resp.tag = MESSAGE_RESPONSE;
    resp.request_id = req_msg->request_id;
    resp.data.response.function_tag = req->function_tag;

    switch (req->function_tag) {
    case FUNCTION_X:
        resp.data.response.result.result_function_x = linux_function_x(req->params.function_x.param);
        log_info("Handled FUNCTION_X request id=%d param=%d result=%.3f", req_msg->request_id, req->params.function_x.param, resp.data.response.result.result_function_x);
        break;
    case FUNCTION_Y:
        resp.data.response.result.result_function_y = linux_function_y(req->params.function_y.param);
        log_info("Handled FUNCTION_Y request id=%d param=%.3f result=%d", req_msg->request_id, req->params.function_y.param, resp.data.response.result.result_function_y);
        break;
    default:
        errno = EEXAMPLE_PROTOCOL;
        log_warn("Ignoring unsupported Linux function tag %s for request id=%d", function_tag_to_string(req->function_tag), req_msg->request_id);
        return -1;
    }

    burn_time_pretending_to_do_stuff(80, 500);
    return send_msg(get_rpmsg_fd(), &resp, sizeof(resp));
}

static int process_incoming_message(const MESSAGE *msg)
{
    switch (msg->tag) {
    case MESSAGE_REQUEST:
        return handle_request_linux(msg);
    case MESSAGE_COMMAND:
        return handle_command_linux(msg);
    case MESSAGE_PING:
        log_info("Received ping from R5");
        return 0;
    case MESSAGE_RESPONSE:
        log_warn("Received orphaned response id=%d tag=%s", msg->request_id, function_tag_to_string(msg->data.response.function_tag));
        return 0;
    default:
        errno = EEXAMPLE_PROTOCOL;
        log_warn("Ignoring message with unknown tag value %u", (unsigned int)msg->tag);
        return -1;
    }
}

static int wait_for_response(uint32_t request_id, FUNCTION_TAG expected_tag, MESSAGE *response, unsigned int timeout_ms)
{
    uint64_t deadline_ms = get_monotonic_ms() + timeout_ms;

    while (get_monotonic_ms() < deadline_ms) {
        MESSAGE msg = {0};
        int recv_len = 0;

        if (recv_msg(get_rpmsg_fd(), &msg, sizeof(msg), &recv_len) != 0) {
            return -1;
        }

        if (recv_len == 0) {
            sleep_ms(IDLE_POLL_DELAY_MS);
            continue;
        }

        if (recv_len != (int)sizeof(MESSAGE)) {
            errno = EEXAMPLE_PROTOCOL;
            log_warn("Ignoring short RPMSG read (%d/%zu)", recv_len, sizeof(MESSAGE));
            continue;
        }

        if (msg.tag == MESSAGE_RESPONSE) {
            if (msg.request_id != request_id) {
                log_warn("Saw response for request id=%d while waiting on id=%d", msg.request_id, request_id);
                continue;
            }
            if (msg.data.response.function_tag != expected_tag) {
                errno = EEXAMPLE_PROTOCOL;
                log_err("Response id=%d had unexpected tag %s while waiting for %s", msg.request_id, function_tag_to_string(msg.data.response.function_tag), function_tag_to_string(expected_tag));
                return -1;
            }

            *response = msg;
            return 0;
        }

        if (process_incoming_message(&msg) != 0 && rpmsg_connected) {
            return -1;
        }
    }

    errno = EEXAMPLE_TIMEOUT;
    log_warn("Timed out waiting for response id=%d tag=%s", request_id, function_tag_to_string(expected_tag));
    return -1;
}

static int send_blocking_request(MESSAGE *request, MESSAGE *response, unsigned int timeout_ms)
{
    if (send_msg(get_rpmsg_fd(), request, sizeof(*request)) != 0) {
        return -1;
    }

    log_info("Sent blocking request id=%d tag=%s", request->request_id, function_tag_to_string(request->data.request.function_tag));
    return wait_for_response(request->request_id, request->data.request.function_tag, response, timeout_ms);
}

static REMOTE_RETURN call_function_a_blocking(int a, int b, int *result_out)
{
    MESSAGE req = {0};
    MESSAGE response = {0};

    req.tag = MESSAGE_REQUEST;
    req.request_id = get_next_req_id();
    req.data.request.function_tag = FUNCTION_A;
    req.data.request.params.function_a.a = a;
    req.data.request.params.function_a.b = b;

    if (send_blocking_request(&req, &response, TIMEOUT_DEFAULT_MS) != 0) {
        return (REMOTE_RETURN){ .error = errno, .rt = NULL };
    }

    *result_out = response.data.response.result.result_function_a;
    log_info("Response id=%d tag=%s result=%d", response.request_id, function_tag_to_string(response.data.response.function_tag), *result_out);
    return (REMOTE_RETURN){ .error = 0, .rt = result_out };
}

static REMOTE_RETURN call_function_b_blocking(float a, float b, float c, float *result_out)
{
    MESSAGE req = {0};
    MESSAGE response = {0};

    req.tag = MESSAGE_REQUEST;
    req.request_id = get_next_req_id();
    req.data.request.function_tag = FUNCTION_B;
    req.data.request.params.function_b.a = a;
    req.data.request.params.function_b.b = b;
    req.data.request.params.function_b.c = c;

    if (send_blocking_request(&req, &response, TIMEOUT_DEFAULT_MS) != 0) {
        return (REMOTE_RETURN){ .error = errno, .rt = NULL };
    }

    *result_out = response.data.response.result.result_function_b;
    log_info("Response id=%d tag=%s result=%.3f", response.request_id, function_tag_to_string(response.data.response.function_tag), *result_out);
    return (REMOTE_RETURN){ .error = 0, .rt = result_out };
}

static REMOTE_RETURN send_command_a_nonblocking(int a, int b)
{
    MESSAGE cmd = {0};

    cmd.tag = MESSAGE_COMMAND;
    cmd.request_id = get_next_req_id();
    cmd.data.request.function_tag = FUNCTION_A;
    cmd.data.request.params.function_a.a = a;
    cmd.data.request.params.function_a.b = b;

    if (send_msg(get_rpmsg_fd(), &cmd, sizeof(cmd)) != 0) {
        return (REMOTE_RETURN){ .error = errno, .rt = NULL };
    }

    log_info("Sent non-blocking command id=%d tag=%s", cmd.request_id, function_tag_to_string(cmd.data.request.function_tag));
    return (REMOTE_RETURN){ .error = 0, .rt = NULL };
}

static int drain_pending_messages(unsigned int max_messages)
{
    unsigned int processed = 0;

    while (processed < max_messages) {
        MESSAGE msg = {0};
        int recv_len = 0;

        if (recv_msg(get_rpmsg_fd(), &msg, sizeof(msg), &recv_len) != 0) {
            return -1;
        }

        if (recv_len == 0) {
            return 0;
        }

        if (recv_len != (int)sizeof(MESSAGE)) {
            errno = EEXAMPLE_PROTOCOL;
            log_warn("Ignoring short RPMSG read (%d/%zu)", recv_len, sizeof(MESSAGE));
            processed++;
            continue;
        }

        if (process_incoming_message(&msg) != 0 && rpmsg_connected) {
            return -1;
        }

        processed++;
    }

    return 0;
}

static int run_demo_cycle(void)
{
    int function_a_result = 0;
    float function_b_result = 0.0f;

    if (call_function_a_blocking(5, 3, &function_a_result).error != 0) {
        return -1;
    }

    burn_time_pretending_to_do_stuff(5, 200);
    if ((get_random_u32() % 5U) == 0U) {
        return 0;
    }

    if (call_function_b_blocking(1.1f, 2.2f, 3.3f, &function_b_result).error != 0) {
        return -1;
    }

    burn_time_pretending_to_do_stuff(5, 200);
    if ((get_random_u32() % 5U) == 0U) {
        return 0;
    }

    if (call_function_b_blocking(1.1f, 2.2f, 3.3f, &function_b_result).error != 0) {
        return -1;
    }

    burn_time_pretending_to_do_stuff(5, 200);
    if ((get_random_u32() % 5U) == 0U) {
        return 0;
    }

    if (send_command_a_nonblocking(10, 20).error != 0) {
        return -1;
    }

    return 0;
}

int rpmsg_char_example_main(void)
{
    if (!cleanup_registered) {
        atexit(rpmsg_cleanup_atexit);
        cleanup_registered = true;
    }

    log_info("Linux RPMSG example loop started");
    while (1) {
        if (rpmsg_try_connect() != 0) {
            sleep_ms(250U);
            continue;
        }

        if (drain_pending_messages(MAX_PENDING_MESSAGES_PER_CYCLE) != 0) {
            if (!rpmsg_connected) {
                continue;
            }
            sleep_ms(IDLE_POLL_DELAY_MS);
            continue;
        }

        if (run_demo_cycle() != 0) {
            if (!rpmsg_connected) {
                continue;
            }
            sleep_ms(IDLE_POLL_DELAY_MS);
            continue;
        }

        burn_time_pretending_to_do_stuff(800, 1200);
    }
}