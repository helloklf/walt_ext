// SPDX-License-Identifier: GPL-3.0-or-later
/* Read-only diagnostic: print WALT tunables/kobject pointers to derive the real
 * kobject offset on a NEW device. Creates NO sysfs files; init returns an error
 * so the module auto-unloads (no zombie). Use during porting (see README).
 *
 * It prints governor_data, tunables=*(gd+TL_GD_TUNABLES_OFF), then scans candidate
 * kobject offsets in tunables: a valid kobject has a kernel-pointer name@+0 and
 * ktype@+40. The offset whose name/ktype look like kernel addresses is the kobject.
 */
#include "walt_kern.h"
#include "device.h"
#define TAG "walt_diag"
PLK_PLT_PAD;
extern struct plk_module wd_this_module __asm__("__this_module");

extern int init_module(void);
extern void cleanup_module(void);

int init_module(void)
{
	struct cpufreq_policy *pol = cpufreq_cpu_get(0);
	void *gd, *ta;
	int off;

	if (!pol) {
		pr_err("[%s] no policy0\n", TAG);
		return -19;
	}
	gd = pol->governor_data;
	pr_info("[%s] policy=%px governor_data=%px governor=%px\n",
		TAG, pol, gd, pol->governor);
	if (!gd) {
		cpufreq_cpu_put(pol);
		return -19;
	}
	ta = *(void **)((char *)gd + TL_GD_TUNABLES_OFF);
	pr_info("[%s] tunables=*(gd+0x%x)=%px\n", TAG, TL_GD_TUNABLES_OFF, ta);
	if (ta) {
		for (off = 0; off <= 0x100; off += 0x10) {
			void *ktype = *(void **)((char *)ta + off + 40);
			void *name = *(void **)((char *)ta + off);
			pr_info("[%s] ta+0x%03x: name=%px ktype(+40)=%px\n",
				TAG, off, name, ktype);
		}
	}
	cpufreq_cpu_put(pol);
	pr_info("[%s] done (auto-unload)\n", TAG);
	return -22; /* force auto-unload, no zombie */
}

void cleanup_module(void) { }

PLK_MODULE_INFO(description, "WALT kobject offset diagnostic");
PLK_MODULE_INFO(license, "GPL");
PLK_MODULE_INFO(name, "walt_diag");
PLK_MODULE_INFO(depends, "");
PLK_MODULE_INFO(vermagic, PLK_VERMAGIC);

__visible struct plk_module wd_this_module
__asm__("__this_module")
__section(".gnu.linkonce.this_module") = {
	.name = "walt_diag",
	.init = init_module,
	.exit = cleanup_module,
};

#include "crcs.h"