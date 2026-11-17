/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Imported-symbol CRCs for OnePlus PYB110 (OP64E1L1), 6.18.21-android17-5.
 * Harvested from this device's own modules (tools/extract_crcs.py):
 * sched-walt.ko / cpufreq_bouncing.ko / qti_cpufreq_cdev.ko (vendor_boot
 * ramdisk) plus aiff_perf.ko (sysfs_*) and inte.ko (kprobe) as donors.
 *
 * All values are byte-identical to hongkong/6.18.21 — KMI frozen within the
 * android17-5 generation (module_layout 0x60801db3 etc.).
 */
static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x16b5b21d, "_printk" },
	{ 0x60801db3, "module_layout" },
	{ 0x8a7493b2, "memcpy" },
	{ 0x110f4d3a, "memset" },
	{ 0xd272d446, "__stack_chk_fail" },
	{ 0x90d19584, "strlen" },
	{ 0xeac1e857, "strsep" },
	{ 0xafb18b07, "kstrtouint" },
	{ 0xf4386284, "scnprintf" },
	{ 0x2776b0a3, "cpufreq_cpu_get" },
	{ 0x1ea526c7, "cpufreq_cpu_put" },
	{ 0xc332eb07, "sysfs_create_group" },
	{ 0x16ff3e85, "sysfs_remove_group" },
	{ 0x100f01ea, "register_kprobe" },
	{ 0x7ec06b6b, "unregister_kprobe" },
	{ 0xae2dbea2, "param_ops_uint" },
};
