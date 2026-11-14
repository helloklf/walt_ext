/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Imported-symbol CRCs for Xiaomi 18 Pro (M610BB / hongkong), 6.18.21-android17-5.
 * Harvested from device modules: sched-walt.ko, hung_task_enhance.ko,
 * adsp_loader_dlkm.ko, dwc3-msm.ko (tools/extract_crcs.py).
 *
 * android17-5 KMI differs from android16-6: module_layout, cpufreq_cpu_get/put,
 * kobject_init_and_add, register_kprobe, sysfs_*, param_ops_* all changed;
 * printk/string/mem functions kept the same CRC (their types are unchanged).
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