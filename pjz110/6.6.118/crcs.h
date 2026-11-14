/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Imported-symbol CRCs for OnePlus 13 (PJZ110), 6.6.118-android15-8.
 * Harvested from device modules: sched-walt.ko, inte.ko, adsp_loader_dlkm.ko,
 * camera.ko (tools/extract_crcs.py).
 *
 * android15-8 KMI is entirely different from android16-6: every CRC differs
 * (including printk/string/mem). Do not reuse 6.12 values here.
 */
static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x92997ed8, "_printk" },
	{ 0x4e276f37, "module_layout" },
	{ 0x4829a47e, "memcpy" },
	{ 0xdcb764ad, "memset" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0x98cf60b3, "strlen" },
	{ 0x85df9b6c, "strsep" },
	{ 0x3b6c41ea, "kstrtouint" },
	{ 0x96848186, "scnprintf" },
	{ 0x72c90142, "cpufreq_cpu_get" },
	{ 0x45930102, "cpufreq_cpu_put" },
	{ 0x31b8c566, "sysfs_create_group" },
	{ 0x3ff466d3, "sysfs_remove_group" },
	{ 0x0472cf3b, "register_kprobe" },
	{ 0xeb78b1ed, "unregister_kprobe" },
	{ 0x460da8f5, "param_ops_uint" },
};