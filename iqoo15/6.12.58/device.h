/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Device/vendor-specific config: iQOO 15 (V2505A / PD2505, vivo OriginOS 7, SM8850),
 * kernel 6.12.58-android16-6-gdd6fd5c98918-abogki543627742-4k, WALT in sched-walt.ko.
 *
 * WALT code is essentially identical to OnePlus 15 (same Qualcomm base): same
 * zone_max_util_pct_store / update_util_inflate_factor function sizes, same kCFI
 * hashes, same KMI CRCs (module_layout 0x797f2b3e). Offsets below are taken from
 * the OnePlus port and CONFIRMED via walt_diag on this device.
 *
 *   governor_data = *(policy + 0x58)         (a wrapper struct)
 *   tunables      = *(governor_data + 0x28)
 *   kobject       = tunables + 0x00          (name@0, ktype@40)
 *   hispeed_load  = tunables + 0xb0
 *   zones         = tunables + 0xf8          ({u32 freq; u32 pct;}[10], freq==-1 ends)
 *
 * NOTE: zones offset is 0xf8 here vs 0xe0 on OnePlus 15 — vivo's tunables struct
 * has 0x18 extra bytes before the zone array. Confirmed via walt_diag (live mem)
 * AND zone_max_util_pct_show/store disassembly ([x,#0xf8]/[x,#0xfc]).
 */
#ifndef WALT_DEVICE_H
#define WALT_DEVICE_H

/* vermagic of the loaded sched-walt.ko (vivo relaxes vermagic to KMI generation;
 * module_layout CRC must match — see crcs.h). */
#define PLK_VERMAGIC \
	"6.12.58-android16-6-gfd60568f1f3a-4k SMP preempt mod_unload modversions aarch64"

/* WALT tunables offsets (vendor-specific; confirmed via walt_diag + disasm) */
#define TL_GD_TUNABLES_OFF	0x28
#define TL_KOBJ_OFF		0x00
#define TL_ZONES_OFF		0xf8
#define TL_MAX_ZONES		10

#define ZONE_STORE_SYM		"zone_max_util_pct_store"
#define GOV_NAME		"walt"
#define FALLBACK_MAX_FREQ	5000000u
#define SELFTEST_RESTORE	"2611200 90"
#define DEVICE_DESC		"iQOO 15 (V2505A, SM8850) WALT governor"

#endif /* WALT_DEVICE_H */