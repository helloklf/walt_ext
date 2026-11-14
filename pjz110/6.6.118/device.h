/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Device/vendor-specific config: OnePlus 13 (PJZ110, ColorOS 17, Snapdragon 8 Elite
 * SM8750), kernel 6.6.118-android15-8-g9bc34d5b0c79-abogki537459655-4k.
 * WALT in sched-walt.ko (vendor_boot ramdisk, [permanent]).
 *
 * 6.6 GKI (android15): struct module 1536/init@392/exit@1464; kprobe 128 and
 * attribute_group attrs@0x18 same as 6.12. kCFI type-hash scheme differs from
 * android16 (see config.sh). BTI only (no PAC).
 *
 * WALT tunables offsets — confirmed via walt_diag + zone_show disassembly:
 *   governor_data = *(policy + 0x58)
 *   tunables      = *(governor_data + 0x28)
 *   kobject       = tunables + 0x00   (name@0, ktype@40)
 *   zones         = tunables + 0xe0   ({u32 freq; u32 pct;}[10], freq==-1 ends)
 *
 * zones offset same as OnePlus 15 (0xe0); iQOO 0xf8, Xiaomi 18 Pro 0xd8 differ.
 */
#ifndef WALT_DEVICE_H
#define WALT_DEVICE_H

#define PLK_VERMAGIC \
	"6.6.118-android15-8-o-g1524d9b076be-4k SMP preempt mod_unload modversions aarch64"

/* WALT tunables offsets — confirmed via walt_diag + disasm */
#define TL_GD_TUNABLES_OFF	0x28
#define TL_KOBJ_OFF		0x00
#define TL_ZONES_OFF		0xe0
#define TL_MAX_ZONES		10

#define ZONE_STORE_SYM		"zone_max_util_pct_store"
/* android15/6.6 zone_store requires input frequencies in descending order
 * (it reverses them to ascending storage internally). Verified by probing. */
#define TL_ZONES_DESCENDING	1
/* zone_store is write-once per cluster (guarded by this .bss flag array);
 * clear it before each write so repeated writes take effect. */
#define TL_ZONE_GUARD_SYM	"write_once_zone_max_util_pct_cluster"
#define TL_ZONE_GUARD_COUNT	4
#define GOV_NAME		"walt"
#define FALLBACK_MAX_FREQ	5000000u
#define SELFTEST_RESTORE	"1324800 80"
/* policy0 OPPs include 1363200 / 1996800 (1200000 / 1900000 are not valid here) */
#define SELFTEST_CFG		"80 1363200:85 1996800:90"
#define DEVICE_DESC		"OnePlus 13 (PJZ110, Snapdragon 8 Elite) WALT governor"

#endif /* WALT_DEVICE_H */