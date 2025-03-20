/*
 * Copyright (c) 2020 Texas Instruments Incorporated - https://www.ti.com
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
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

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "rpmsg_char_internal.h"

int get_child_dir_suffix(char *fpath, const char *child_name_pattern,
			 unsigned int *suffix)
{
	struct dirent *iter;
	DIR *parent;
	int ret = -ENODEV;

	parent = opendir(fpath);
	if (!parent)
		return -errno;

	while ((iter = readdir(parent))) {
		if (iter->d_type == DT_DIR &&
		    sscanf(iter->d_name, child_name_pattern, suffix)) {
			ret = 0;
			break;
		}
	}

	closedir(parent);
	return ret;
}

int get_child_dir_pattern(char *fpath, const char *child_name_pattern,
			 char *dir_name)
{
	struct dirent *iter;
	DIR *parent;
	int ret = -ENODEV;

	parent = opendir(fpath);
	if (!parent)
		return -errno;

	while ((iter = readdir(parent))) {
		if (iter->d_type == DT_DIR &&
			(strncmp(iter->d_name,child_name_pattern,strlen(child_name_pattern))
			 == 0)) {
			strcpy(dir_name,iter->d_name);
			ret = 0;
			break;
		}
	}

	closedir(parent);
	return ret;
}

int file_read_string(char *fpath, char *buf, int size)
{
	int fd, bytes;

	fd = open(fpath, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "could not open %s: errno = %d\n",
			fpath, errno);
		return -errno;
	}

	bytes = read(fd, buf, size);
	close(fd);
	if (bytes <= 0) {
		fprintf(stderr, "could not read %s: errno = %d\n",
			fpath, errno);
		return -EIO;
	}
	if (bytes >= size) {
		fprintf(stderr, "%d bytes read from %s are larger than size %d\n",
			bytes, fpath, size);
		return -EIO;
	}

	/* suppress the newline */
	buf[bytes - 1] = '\0';

	return bytes;
}

int file_read_value(char *fpath)
{
	char buf[32];
	int ret;

	ret = file_read_string(fpath, buf, sizeof(buf));
	if (ret < 0)
		return ret;

	return strtol(buf, NULL, 0);
}

int check_dir(char *dirpath)
{
	struct stat s;

	if (stat(dirpath, &s) == 0 && S_ISDIR(s.st_mode))
		return 0;

	return -ENOENT;
}

/* Returns a pointer to allocated memory, needs to be freed once done */
char *file_deref_link(char *fpath, char *link_name)
{
	char path[512] = { 0 };
	char rel_path[256] = { 0 };
	int n, nr;

	n = snprintf(path, 255, "%s/%s", fpath, link_name);
	if (n < 0 || n >= 255) {
		fprintf(stderr, "%s: could not create full path string\n",
			__func__);
		return NULL;
	}

	nr = readlink(path, rel_path, sizeof(rel_path) - 1);
	if (nr < 0) {
		fprintf(stderr, "%s: readlink failed for %s\n", __func__, path);
		return NULL;
	}

	if (n + nr >= 512) {
		fprintf(stderr, "%s: full relative path exceeds buffer size, n = %d nr = %d\n",
			__func__, n, nr);
		return NULL;
	}

	memset(path, 0, sizeof(path));
	sprintf(path, "%s/%s", fpath, rel_path);

	return realpath(path, NULL);
}
