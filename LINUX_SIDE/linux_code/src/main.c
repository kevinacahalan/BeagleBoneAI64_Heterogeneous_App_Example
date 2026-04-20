#include <errno.h>

#include "../../../SHARED_CODE/include/dbg.h"
#include <rpmsg_char_example.h>

int main(int argc, char **argv)
{
    int ret;

    (void)argc;
    (void)argv;

    log_info("Starting Linux RPMSG example");
    ret = rpmsg_char_example_main();
    if (ret != 0) {
        errno = ret;
        log_err("Linux RPMSG example exited with status %d", ret);
        return ret;
    }

    log_info("Linux RPMSG example exited cleanly");
    return 0;
}
