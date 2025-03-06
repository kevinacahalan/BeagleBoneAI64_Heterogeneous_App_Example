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

#ifndef __RPMSG_CHAR_INTERNAL_H__
#define __RPMSG_CHAR_INTERNAL_H__

#include <rproc_id.h>

#if defined(__cplusplus)
extern "C" {
#endif

struct rproc_map {
	enum rproc_id id;
	char *rproc_name;
};

struct soc_rprocs {
	int num_rprocs;
	const struct rproc_map *map;
};

/* soc.c */
int _rpmsg_char_find_soc_family(const char *soc_name, struct soc_rprocs *soc);

/* utils.c */
char *find_child_dir_by_name(DIR *parent, char *child_name);
int get_child_dir_suffix(char *fpath, const char *child_name_pattern,
			 unsigned int *suffix);
int get_child_dir_pattern(char *fpath, const char *child_name_pattern,
			 char *dir);
char *str_join(const char *fmt, ...);
int file_read_string(char *fpath, char *buf, int size);
int file_read_value(char *fpath);
int check_dir(char *dirpath);
char *file_deref_link(char *fpath, char *link_name);

#if defined(__cplusplus)
}
#endif

#endif
