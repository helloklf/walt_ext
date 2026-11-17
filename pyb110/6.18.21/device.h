/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Device/vendor-specific config: OnePlus PYB110 (OP64E1L1), ColorOS 17
 * (Android 17), kernel 6.18.21-android17-5-g7fcbffcad3e1-ab16353812-4k.
 * WALT in sched-walt.ko (vendor_boot ramdisk, [permanent]).
 *
 * 6.18 GKI struct layouts verified against this device's BTF (identical to
 * hongkong/6.18.21): module 1664/init@376/exit@1536, kprobe 120,
 * attribute_group attrs@0x20, cpufreq_policy governor_data@0x58, kernel_param 40.
 *
 * WALT tunables offsets — from waltgov_init / zone_max_util_pct_{show,store}
 * disassembly of this device's sched-walt.ko (walt_diag live-memory check
 * still pending; no write-once guard, no descending-order quirk):
 *   governor_data = *(policy + 0x58)
 *   tunables      = *(governor_data + 0x28)
 *   kobject       = tunables + 0x00   (name@0, ktype@40)
 *   zones         = tunables + 0xd8   ({u32 freq; u32 pct;}[10], freq==-1 ends)
 *
 * zones offset differs from OnePlus 6.12 (plk110 0xe0); same as Xiaomi 6.18
 * (hongkong 0xd8).
 */
#ifndef WALT_DEVICE_H
#define WALT_DEVICE_H

/* vermagic taken verbatim from this device's sched-walt.ko (ColorOS relaxes
 * vermagic to KMI under MODVERSIONS; module_layout CRC is the real gate). */
#define PLK_VERMAGIC \
	"6.18.21-android17-5-o-maybe-dirty-4k SMP preempt mod_unload modversions aarch64"

/* WALT tunables offsets — from disassembly, walt_diag confirmation pending */
#define TL_GD_TUNABLES_OFF	0x28
#define TL_KOBJ_OFF		0x00
#define TL_ZONES_OFF		0xd8
#define TL_MAX_ZONES		10

#define ZONE_STORE_SYM		"zone_max_util_pct_store"
#define GOV_NAME		"walt"
/* policy6 cpuinfo_max_freq is 5011200 on this device */
#define FALLBACK_MAX_FREQ	5011200u
#define SELFTEST_RESTORE	"2611200 90"
#define DEVICE_DESC		"OnePlus PYB110 (OP64E1L1) WALT governor"

#endif /* WALT_DEVICE_H */
