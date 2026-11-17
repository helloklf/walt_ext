/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Device/vendor-specific config: OnePlus ACE7 Pro (PMS110 / OP64F1L1), ColorOS 17
 * (Android 17), kernel 6.12.69-android16-6-g5fd39602326e-ab16372235-4k.
 * WALT in sched-walt.ko (vendor_boot ramdisk, [permanent]).
 *
 * 6.12 GKI struct layouts verified against this device's BTF (identical to
 * plk110/6.12.69): module 1600/init@392/exit@1528, kprobe 128,
 * attribute_group attrs@0x18, cpufreq_policy governor_data@0x58, kernel_param 40.
 *
 * WALT tunables offsets — from waltgov_init / zone_max_util_pct_{show,store}
 * disassembly of this device's sched-walt.ko (no write-once guard, no
 * descending-order quirk; same as plk110):
 *   governor_data = *(policy + 0x58)
 *   tunables      = *(governor_data + 0x28)
 *   kobject       = tunables + 0x00   (name@0, ktype@40)
 *   zones         = tunables + 0xe0   ({u32 freq; u32 pct;}[10], freq==-1 ends)
 */
#ifndef WALT_DEVICE_H
#define WALT_DEVICE_H

/* vermagic taken verbatim from this device's sched-walt.ko (ColorOS relaxes
 * vermagic to KMI under MODVERSIONS; module_layout CRC is the real gate). */
#define PLK_VERMAGIC \
	"6.12.69-android16-6-o-g797637f5bc7e-4k SMP preempt mod_unload modversions aarch64"

/* WALT tunables offsets — from disassembly */
#define TL_GD_TUNABLES_OFF	0x28
#define TL_KOBJ_OFF		0x00
#define TL_ZONES_OFF		0xe0
#define TL_MAX_ZONES		10

#define ZONE_STORE_SYM		"zone_max_util_pct_store"
#define GOV_NAME		"walt"
#define FALLBACK_MAX_FREQ	5000000u
#define SELFTEST_RESTORE	"2611200 90"
#define DEVICE_DESC		"OnePlus ACE7 Pro (PMS110) WALT governor"

#endif /* WALT_DEVICE_H */
