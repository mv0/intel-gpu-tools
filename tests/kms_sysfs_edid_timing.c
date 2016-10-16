/*
 * Copyright © 2016 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "igt.h"
#include "igt_debugfs.h"
#include "igt_sysfs.h"
#include "igt_core.h"

#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>

IGT_TEST_DESCRIPTION("This check the time we take to read the content of all "
		     "the possible connectors. Without the edid -ENXIO patch "
		     "(http://permalink.gmane.org/gmane.comp.video.dri.devel/62083), "
		     "we sometimes take a *really* long time. "
		     "So let's just check for some reasonable timing here");

#define MSEC_PER_SEC (1000)
#define USEC_PER_SEC (1000*MSEC_PER_SEC)
#define NSEC_PER_SEC (1000*USEC_PER_SEC)

static uint64_t
ms_elapsed(const struct timespec *start, const struct timespec *end)
{
	uint64_t x = NSEC_PER_SEC * (end->tv_sec - start->tv_sec) +
		    (end->tv_nsec - start->tv_nsec);
	return (x / USEC_PER_SEC);
}

igt_simple_main
{
	int dir = igt_sysfs_open(-1, NULL);
	DIR *dirp = fdopendir(dir);
	int fds[32];

	struct dirent *de;
	struct stat st;
	int i, fd = 0;
	struct timespec start, stop;

	memset(fds, -1, sizeof(fds));

	while ((de = readdir(dirp))) {

		if (*de->d_name == '.')
			continue;

		if (fstatat(dir, de->d_name, &st, 0))
			continue;

		if (S_ISDIR(st.st_mode) &&
		    strncmp(de->d_name, "card0-", 6) == 0) {
			int dfd = openat(dir, de->d_name, O_RDONLY);
			igt_assert(!(dfd < 0));
			fds[fd++] = dfd;
		}
	}

	clock_gettime(CLOCK_MONOTONIC, &start);
	for (i = 0; i < fd - 1; i++) {
		igt_assert(igt_sysfs_get(fds[i], "status") != NULL);
		close(fds[i]);
	}
	clock_gettime(CLOCK_MONOTONIC, &stop);

	if (ms_elapsed(&start, &stop) > 600) {
		igt_fail(IGT_EXIT_FAILURE);
	}

	closedir(dirp);
}
