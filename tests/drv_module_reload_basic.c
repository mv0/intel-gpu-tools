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
#include <sys/utsname.h>
#include <linux/limits.h>
#include <signal.h>

#include <libkmod.h>
#include <proc/readproc.h>

#define MODULES_NO	64

struct k_mod {
	char **modules;
	int no_modules;
	int allocated;
};

const char *vt_path = "/sys/class/vtconsole";
static const char *basic_tests[] = {
	"gem_alive", "gem_exec_store", NULL
};

/* used to retrive the dirname where this is executed */
char dirbasename[PATH_MAX];

static int
lsmod_get_modules(char ***modules, int *no_modules, int *allocated)
{
	struct kmod_list *mod, *list;
	struct kmod_ctx	*ctx;
	int m = 0;
	char **rmodules = NULL;

	*allocated = MODULES_NO;

	ctx = kmod_new(NULL, NULL);
	igt_assert(ctx != NULL);

	if (kmod_module_new_from_loaded(ctx, &list) < 0) {
		kmod_unref(ctx);
		return -1;
	}

	rmodules = calloc(*allocated, sizeof(char *));

	kmod_list_foreach(mod, list) {
		struct kmod_module *kmod = kmod_module_get_module(mod);

		if (m >= *allocated) {
			*allocated = *allocated * 2;
			rmodules = realloc(rmodules, *allocated * sizeof(char *));
		}

		rmodules[m] = malloc(strlen(kmod_module_get_name(kmod)) + 1);
		memset(rmodules[m], 0, strlen(kmod_module_get_name(kmod)) + 1);
		memcpy(rmodules[m], kmod_module_get_name(kmod),
				strlen(kmod_module_get_name(kmod)));
		m++;
		kmod_module_unref(kmod);
	}

	kmod_module_unref_list(list);
	kmod_unref(ctx);

	*no_modules = m--;
	*modules = rmodules;

	return 0;
}

static void
lsmod_free_modules(struct k_mod *k_mod)
{
	int i;

	for (i = 0; i < k_mod->no_modules; i++)
		free(k_mod->modules[i]);

	free(k_mod->modules);
}

static bool
lsmod_has_module(const char *mod_name)
{
	struct kmod_list *mod, *list;
	struct kmod_ctx	*ctx;

	ctx = kmod_new(NULL, NULL);
	igt_assert(ctx != NULL);

	if (kmod_module_new_from_loaded(ctx, &list) < 0) {
		kmod_unref(ctx);
		return false;
	}

	kmod_list_foreach(mod, list) {
		struct kmod_module *kmod = kmod_module_get_module(mod);
		const char *kmod_name = kmod_module_get_name(kmod);

		if (!strncasecmp(kmod_name, mod_name, strlen(kmod_name))) {
			kmod_module_unref(kmod);
			break;
		}

		kmod_module_unref(kmod);
	}

	kmod_module_unref_list(list);
	kmod_unref(ctx);

	return true;
}


static bool
module_in_use(struct kmod_module *kmod)
{
	int err;

	err = kmod_module_get_initstate(kmod);

	/* if compiled builtin or does not exist */
	if (err == KMOD_MODULE_BUILTIN || err < 0) {
		return false;
	}

	if (kmod_module_get_refcnt(kmod) != 0 ||
	    kmod_module_get_holders(kmod) != NULL)
		return true;


	return false;
}

static int
insmod(const char *mod_name, const char *opts)
{
	struct kmod_ctx	*ctx;
	struct kmod_module *kmod;
	int err = 0;

	ctx = kmod_new(NULL, NULL);
	igt_assert(ctx != NULL);


	err = kmod_module_new_from_name(ctx, mod_name, &kmod);
	if (err < 0) {
		goto out;
	}

	err = kmod_module_insert_module(kmod, 0, opts);
	if (err < 0) {
		switch (err) {
		case -EEXIST:
			igt_info("Module %s already inserted\n",
					kmod_module_get_name(kmod));
			return -1;
		case -ENOENT:
			igt_info("Unknown symbol in module %s or"
				 "unknown parameter",
				 kmod_module_get_name(kmod));
			return -1;
		default:
			igt_info("Could not insert %s (%s)\n",
				  kmod_module_get_name(kmod),
				  strerror(-err));
			return -1;
		}
	}
out:
	kmod_module_unref(kmod);
	kmod_unref(ctx);

	return err;
}

static int
rmmod(const char *mod_name, bool force)
{
	struct kmod_ctx	*ctx;
	struct kmod_module *kmod;
	int err, flags = 0;

	ctx = kmod_new(NULL, NULL);
	igt_assert(ctx != NULL);

	err = kmod_module_new_from_name(ctx, mod_name, &kmod);
	if (err < 0) {
		igt_info("Could not use module %s (%s)\n", mod_name,
				strerror(-err));
		err = -1;
		goto out;
	}

	if (module_in_use(kmod)) {
		igt_info("Module %s in use\n", mod_name);
		err = -1;
		goto out;
	}

	if (force) {
		flags |= KMOD_REMOVE_FORCE;
	}

	err = kmod_module_remove_module(kmod, flags);
	if (err < 0) {
		igt_info("Could not remove module %s (%s)\n", mod_name,
				strerror(-err));
		err = -1;
	}

out:
	kmod_module_unref(kmod);
	kmod_unref(ctx);

	return err;
}

static int readN(int fd, char *buf, int len)
{
	int total = 0;
	do {
		int ret = read(fd, buf + total, len - total);
		if (ret < 0 && (errno == EINTR || errno == EAGAIN))
			continue;

		if (ret <= 0)
			return total ?: ret;

		total += ret;
	} while (1);
}

static void
kick_fbconn(int type)
{
	DIR *dir;
	struct dirent *de;
	char fb_dev[] = "frame buffer device";

	dir = opendir(vt_path);

	while ((de = readdir(dir))) {
		if (*de->d_name == '.')
			continue;

		if (!strncasecmp(de->d_name, "vtcon", 5)) {
			int fd;
			char dirname_buf[PATH_MAX];
			char buf[512];

			memset(dirname_buf, 0, sizeof(dirname_buf));

			snprintf(dirname_buf, sizeof(dirname_buf), "%s/%s/name",
				 vt_path, de->d_name);

			fd = open(dirname_buf, O_RDONLY);
			igt_assert(fd > 0);

			memset(buf, 0, 512);
			readN(fd, buf, 512);
			if (strstr(buf, fb_dev) == NULL) {
				close(fd);
				continue;
			}

			memset(dirname_buf, 0, sizeof(dirname_buf));

			snprintf(dirname_buf, sizeof(dirname_buf), "%s/%s/bind",
				 vt_path, de->d_name);

			fd = open(dirname_buf, O_WRONLY);
			igt_assert(fd > 0);
			switch (type) {
			case 0:
				write(fd, "0", 1);
				break;
			case 1:
				write(fd, "1", 1);
				break;
			default:
				igt_assert(type == 1 || type == 0);
			}
			close(fd);
		}
	}
	closedir(dir);
}

static int
pkill(int sig, const char *comm)
{
	int err = 0, try = 5;
	PROCTAB *proc;
	proc_t *procd;

	proc = openproc(PROC_FILLCOM | PROC_FILLSTAT | PROC_FILLARG);
	igt_assert(proc != NULL);

	while ((procd = readproc(proc, NULL))) {
		if (procd->cmd &&
		    !strncasecmp(procd->cmd, comm, sizeof(procd->cmd))) {

				/* try a few times in case doesn't work */
				do {
					kill(procd->tid, sig);
				} while (kill(procd->tid, 0) < 0 && try--);

				if (kill(procd->tid, 0) < 0) {
					err = -1;
					freeproc(procd);
				}
				break;
			}

		freeproc(procd);
	}

	closeproc(proc);

	return err;
}

static int
reload(const char *opts_i915)
{
	kick_fbconn(0);

	if (lsmod_has_module("snd_hda_intel")) {

		if (pkill(SIGTERM, "alsactl") == -1) {
			return IGT_EXIT_FAILURE;
		}

		if (rmmod("snd_hda_intel", false) == -1)
			return IGT_EXIT_FAILURE;
	}

	/* gen5 */
	if (lsmod_has_module("intel_ips"))
		rmmod("intel_ips", false);

	if (rmmod("i915", false) == -1) {
		return IGT_EXIT_SKIP;
	}

	rmmod("intel-gtt", false);
	rmmod("drm_kms_helper", false);
	rmmod("drm", false);

	if (lsmod_has_module("i915")) {
		igt_info("WARNING: i915.ko still loaded!");
		return IGT_EXIT_FAILURE;
	} else {
		igt_info("module successfully unloaded");
	}

	if (insmod("i915", opts_i915) == -1) {
		return IGT_EXIT_FAILURE;
	}

	kick_fbconn(1);

	if (insmod("snd_hda_intel", NULL) == -1)
		return IGT_EXIT_FAILURE;

	return IGT_EXIT_SUCCESS;
}

static int
finish_load(char *dirname)
{
	int err;
	char buf[PATH_MAX];
	char *__argv[2];

	memset(buf, 0, PATH_MAX);

	snprintf(buf, sizeof(buf), "%s/tests/%s", dirname, basic_tests[0]);

	__argv[0] = buf;
	__argv[1] = NULL;

	igt_fork(child, 1) {
		err = execv(__argv[0], __argv);
		if (err < 0) {
			igt_info("Failed to exec %s\n",
					basic_tests[0]);
			return IGT_EXIT_FAILURE;
		}
	}
	igt_waitchildren();

	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf), "%s/tests/%s", dirname, basic_tests[1]);

	__argv[0] = buf;
	__argv[1] = NULL;

	igt_fork(child, 1) {
		err = execv(__argv[0], __argv);
		if (err < 0) {
			igt_info("Failed to exec %s\n",
					basic_tests[0]);
			return IGT_EXIT_FAILURE;
		}
	}
	igt_waitchildren();

	return IGT_EXIT_SUCCESS;
}

static void
dirname(char *fn, char *dir)
{
	size_t len;
	char *end = fn + strlen(fn);

	while (end != NULL) {
		if (*end == '/')
			break;
		end--;
	}

	len = end - fn + 1;
	snprintf(dirbasename, len, "%s", fn);
}

int main(int argc, char *argv[])
{
	int i;
	char buf[64];
	int err;

	dirname(argv[0], dirbasename);
	igt_subtest_init_parse_opts(&argc, argv, "", NULL,
				    NULL, NULL, NULL);

	if ((err = reload(NULL)) != IGT_EXIT_SUCCESS)
		igt_fail(err);

	if (!finish_load(dirbasename))
		igt_fail(IGT_EXIT_FAILURE);

	for (i = 0; i < 4; i++) {
		memset(buf, 0, sizeof(buf));
		snprintf(buf, sizeof(buf), "inject_load_failure=%d", i);
		reload(buf);
	}

	if ((err = reload(NULL)) != IGT_EXIT_SUCCESS)
		igt_fail(err);

	igt_exit();
}
