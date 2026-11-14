/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Imported-symbol CRCs for iQOO 15 (V2505A), 6.12.58-android16.
 * Verified identical to the OnePlus 15 (6.12.69) set: same android16-6 GKI KMI
 * (module_layout 0x797f2b3e). Re-harvested/confirmed from iQOO modules:
 * sched-walt.ko / common_hook.ko / adsp_loader_dlkm.ko (extract_crcs.py).
 *
 * Note: modversions hashes the TYPE (e.g. struct kernel_param_ops), not the
 * symbol name, so all kernel_param_ops instances share one CRC.
 */
static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x16b5b21d, "_printk" },
	{ 0x797f2b3e, "module_layout" },
	{ 0x8a7493b2, "memcpy" },
	{ 0x110f4d3a, "memset" },
	{ 0xd272d446, "__stack_chk_fail" },
	{ 0x90d19584, "strlen" },
	{ 0xeac1e857, "strsep" },
	{ 0xafb18b07, "kstrtouint" },
	{ 0xf4386284, "scnprintf" },
	{ 0x27635a3c, "cpufreq_cpu_get" },
	{ 0xe9f20ed5, "cpufreq_cpu_put" },
	{ 0x0e47760e, "sysfs_create_group" },
	{ 0x04d7c425, "sysfs_remove_group" },
	{ 0xcca4d86f, "register_kprobe" },
	{ 0x1d045bd3, "unregister_kprobe" },
	{ 0xd0f4936b, "param_ops_uint" },
};