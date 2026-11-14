/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Device/vendor-specific config: OnePlus 15 (PLK110, SM8850), ColorOS 17,
 * kernel 6.12.69-android16-6-g242e1a07f878-ab15865446-4k, WALT in sched-walt.ko.
 *
 * WALT tunables layout — empirically verified on device via walt_diag:
 *   governor_data = *(policy + 0x58)         (a wrapper struct)
 *   tunables      = *(governor_data + 0x28)  (the real tunables)
 *   kobject       = tunables + 0x00          (name@0, ktype@40)
 *   hispeed_load  = tunables + 0xb0          (u32)
 *   zones         = tunables + 0xe0          ({u32 freq; u32 pct;}[10], freq==-1 ends)
 *
 * When porting to a new device, re-derive these offsets (see README.md /
 * materials/): disassemble the vendor zone_max_util_pct_{show,store} and
 * waltgov_init, or run walt_diag to read live memory.
 */
#ifndef WALT_DEVICE_H
#define WALT_DEVICE_H

/* vermagic proven to load on this device (ColorOS relaxes vermagic to KMI;
 * module_layout CRC must still match — see crcs.h). */
#define PLK_VERMAGIC \
	"6.12.69-android16-6-o-gb79517a1cbef-4k SMP preempt mod_unload modversions aarch64"

/* WALT tunables offsets (vendor-specific) */
#define TL_GD_TUNABLES_OFF	0x28   /* tunables = *(governor_data + this) */
#define TL_KOBJ_OFF		0x00   /* kobject embedded at tunables + this */
#define TL_ZONES_OFF		0xe0   /* zone array at tunables + this */
#define TL_MAX_ZONES		10     /* max zones the vendor parser accepts */

/* vendor symbol that stores a zone string (resolved via transient kprobe) */
#define ZONE_STORE_SYM		"zone_max_util_pct_store"

/* governor name to match (policy->governor->name) */
#define GOV_NAME		"walt"

/* fallback max_freq (kHz) when a policy isn't recorded; above every OPP here */
#define FALLBACK_MAX_FREQ	5000000u

/* selftest: vendor baseline zone string to restore after the test */
#define SELFTEST_RESTORE	"2611200 90"

/* module description fragment */
#define DEVICE_DESC		"OnePlus 15 (PLK110, SM8850) WALT governor"

#endif /* WALT_DEVICE_H */