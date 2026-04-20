#ifndef EXAMPLE_RPMSG_TALK_H
#define EXAMPLE_RPMSG_TALK_H

#include <ai64/bbai64_rpmsg.h>

int setup_r5_example_talk(void);
int example_rpmsg_peer_ready(void);
int teardown_r5_example_talk(void);
int process_linux_messages(int max_messages);

REMOTE_RETURN call_linux_function_x_blocking(int param, float *result_out);
REMOTE_RETURN call_linux_function_y_blocking(float param, int *result_out);
REMOTE_RETURN send_command_x_nonblocking(int param);

#endif