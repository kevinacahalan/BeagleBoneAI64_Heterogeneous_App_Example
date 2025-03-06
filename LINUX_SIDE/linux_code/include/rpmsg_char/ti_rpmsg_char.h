/*
 * Copyright (c) 2020 Texas Instruments Incorporated - https://www.ti.com
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef __TI_RPMSG_CHAR_H__
#define __TI_RPMSG_CHAR_H__

#include <rproc_id.h>

#if defined(__cplusplus)
extern "C" {
#endif

#define RPMSG_DEV_NAME_MAXLEN	32
#define RPMSG_ADDR_ANY                  0xFFFFFFFF
#define RPMSG_RESERVED_ADDRESSES        (1024)
/*!
 * @brief Structure returned during rpmsg_char_open
 *
 * @param fd Opened file descriptor for the local rpmsg endpoint device
 * @param endpt Local Endpoint address used for receiving messages
 */
struct rpmsg_char_dev {
	int fd;
	int endpt;
};

typedef struct rpmsg_char_dev rpmsg_char_dev_t;

/*!
 *  @brief      Function to create and access a rpmsg endpoint device for a
 *              given rpmsg device
 *
 *              This function is to be used to open a communication channel
 *              in a Linux application with any rpmsg device that is bound to
 *              the Linux kernel rpmsg_chrdev driver (rpmsg_char.ko module).
 *
 *              The function returns a pointer to a rpmsg_char_dev structure
 *              with the file descriptor and local end-point address filled in
 *              and usable by an application for a given remoteproc, rpmsg
 *              device (identified by published name from remote processor) and
 *              the remote end-point address used by the rpmsg device.
 *
 *              The opened file descriptor can be used with regular file
 *              operations such as select(), read() and write() functions to
 *              wait, receive and send messages to the rpmsg device on the
 *              remote processor. The messages are received on the local
 *              end-point address.
 *
 *  @param      rproc_id remoteproc enum value identifying a remote processor.
 *                       Value is a local id used by the library
 *
 *  @param      dev_name Name of the rpmsg device name to open. Passing NULL
 *                       will use a default name "rpmsg_chrdev" for the rpmsg
 *                       devices, and identified by remote_endpt. The maximum
 *                       length for the rpmsg device name is as per the virtio
 *                       rpmsg transport, RPMSG_DEV_NAME_MAXLEN (32 bytes)
 *
 *  @param      local_endpt rpmsg local end-point address for the rpmsg device
 *                           to open. Valid value is mandatory
 *
 *  @param      remote_endpt rpmsg remote end-point address for the rpmsg device
 *                           to open. Valid value is mandatory
 *
 *  @param      eptdev_name Name used to create and identify the local endpoint
 *                          device. Needs to be unique across all the Linux
 *                          applications. The maximum length for the
 *                          eptdev_name is RPMSG_DEV_NAME_MAXLEN (32 bytes)
 *
 *  @param      flags Flags used to open the local end-point device
 *
 *  @ret        A valid pointer to a rpmsg_char_dev_t on success, NULL on
 *              failures
 *
 *  @sa         rpmsg_char_close
 */
rpmsg_char_dev_t *rpmsg_char_open(enum rproc_id id, char *dev_name,
				  unsigned int local_endpt,
				  unsigned int remote_endpt,
				  char *eptdev_name, int flags);

/*!
 *  @brief      Function to close and delete a previously created local endpoint
 *              device with rpmsg_char_open
 *
 *              Once this function is called, any regular file operations on the
 *              underlying fd cannot be used. The pointer value is not modified
 *              within the function, but is no longer valid after a successful
 *              return.
 *
 *  @param      rcdev  Pointer to the previously created rpmsg_char_dev_t. The
 *                     handle will be unusable with any of the regular file
 *                     operation functions after a success
 *
 *  @ret        0 on success, a negative value on failure
 *
 *  @sa         rpmsg_char_open
 */
int rpmsg_char_close(rpmsg_char_dev_t *rcdev);

/*!
 *  @brief      Function to initialize the library
 *
 *              This function initializes the library after performing SoC
 *              detection and corresponding initialization. This needs to be
 *              invoked before being able to use any other function. This only
 *              needs to be invoked once in an application, there is no reference
 *              counting. The function also registers signal handlers optionally
 *              (if none are installed prior) to perform any cleanup of stale
 *              rpmsg endpoint devices during abnormal termination of
 *              applications.
 *
 *  @param      soc_name  Name of the SoC family to be used for manual detection
 *                        if kernel doesn't support a socbus device for a
 *                        particular SoC and auto detection fails. Preferred
 *                        usage is just to pass NULL
 *
 *  @ret        0 on success, 1 if already invoked previously, and a negative
 *              value on failure
 *
 *  @sa         rpmsg_char_exit
 */
int rpmsg_char_init(char *soc_name);

/*!
 *  @brief      Function to finalize the library
 *
 *              This function finalizes and performs all the de-initialization
 *              and any cleanup on the library. This is the last function that
 *              needs to be invoked after all usage is done as part of the
 *              application's cleanup. This only need to be invoked once in an
 *              application, there is no reference counting. The function also
 *              needs to be invoked in any application's signal handlers to
 *              perform the necessary cleanup of stale rpmsg endpoint devices.
 *
 *  @ret        None
 *
 *  @sa         rpmsg_char_init
 */
void rpmsg_char_exit(void);

#if defined(__cplusplus)
}
#endif

#endif /* __TI_RPMSG_CHAR_H__ */
