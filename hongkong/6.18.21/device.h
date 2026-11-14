/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Device/vendor-specific config: Xiaomi 18 Pro (M610BB / hongkong, HyperOS 4,
 * Snapdragon 8 Elite 6), kernel 6.18.21-android17-5-g284297031c53-abogki555938958-4k.
 * WALT in sched-walt.ko (vendor_boot ramdisk, [permanent]).
 *
 * 6.18 GKI struct layouts differ from 6.12 (handled in common/walt_kern.h via
 * WALT_KVER=618): module size 1664/init@376/exit@1536, kprobe size 120,
 * attribute_group attrs@0x20. cpufreq_policy offsets unchanged (governor_data@0x58).
 *
 * WALT tunables offsets — confirmed via walt_diag (live memory) + zone_show/store
 * disassembly on this device:
 *   governor_data = *(policy + 0x58)
 *   tunables      = *(governor_data + 0x28)
 *   kobject       = tunables + 0x00   (name@0, ktype@40)
 *   zones         = tunables + 0xd8   ({u32 freq; u32 pct;}[10], freq==-1 ends)
 *
 * zones offset is vendor-specific: OnePlus 0xe0, iQOO 0xf8, Xiaomi 0xd8.
 */
#ifndef WALT_DEVICE_H
#define WALT_DEVICE_H

#define PLK_VERMAGIC \
	"6.18.21-android17-5-g32de97493654-mi-4k SMP preempt mod_unload modversions aarch64"

/* WALT tunables offsets — confirmed via walt_diag + disasm */
#define TL_GD_TUNABLES_OFF	0x28
#define TL_KOBJ_OFF		0x00
#define TL_ZONES_OFF		0xd8
#define TL_MAX_ZONES		10

#define ZONE_STORE_SYM		"zone_max_util_pct_store"
#define GOV_NAME		"walt"
#define FALLBACK_MAX_FREQ	5000000u
#define SELFTEST_RESTORE	"2611200 90"
#define DEVICE_DESC		"Xiaomi 18 Pro (M610BB, Snapdragon 8 Elite 6) WALT governor"

#endif /* WALT_DEVICE_H */