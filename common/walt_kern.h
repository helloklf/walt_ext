/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Generic kernel definitions for walt_tl — targets 6.12 GKI (android16).
 * Device/vendor-specific values (WALT tunables offsets, vermagic, target symbol
 * names, CRCs, kCFI hashes) live in the per-device <device>/<kernel>/ dir
 * (device.h / crcs.h / config.sh), NOT here.
 *
 * Struct layouts are BTF/disasm-verified for 6.12 GKI. When porting to a new
 * KERNEL VERSION, re-verify: struct module size/init/exit offsets, kprobe size,
 * kernel_param size, cpufreq_policy offsets. When porting to a new DEVICE on the
 * same kernel, only device.h/crcs.h/config.sh usually change.
 *
 * Packaging: raw-clang (no kbuild), kCFI normalize-integers, pac-ret (6.12 has
 * NO BTI). See common/build.sh.
 */
#ifndef WALT_KERN_H
#define WALT_KERN_H

/* Kernel version selector, defined by the device config.sh via -DWALT_KVER.
 * 612 = 6.12 GKI (android16), 618 = 6.18 GKI (android17). Struct layouts that
 * differ between versions are guarded below. */
#ifndef WALT_KVER
#define WALT_KVER 612
#endif

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;
typedef unsigned long long u64;
typedef signed char    s8;
typedef int            s32;
typedef unsigned long  size_t;
typedef unsigned long  ulong;
typedef long           ssize_t;
#define NULL ((void *)0)

#define __used    __attribute__((__used__))
#define __section(S) __attribute__((__section__(S)))
#define __aligned(x) __attribute__((__aligned__(x)))
#define __visible __attribute__((used))

extern int _printk(const char *fmt, ...);
#define pr_info(fmt, ...)  _printk("\0016" fmt, ##__VA_ARGS__)
#define pr_err(fmt, ...)   _printk("\0013" fmt, ##__VA_ARGS__)

/* ---- struct module shadow (BTF-verified per kernel version) ----
 * 6.6 : size=1536, name@24, init@392, exit@1464
 * 6.12: size=1600, name@24, init@392, exit@1528
 * 6.18: size=1664, name@24, init@376, exit@1536 */
struct plk_module {
	u32 state;                 /* +0    */
	u32 _pad0;                 /* +4    */
	struct { void *next, *prev; } list; /* +8 */
	char name[56];             /* +24   */
#if WALT_KVER >= 618
	u8 _pad1[376 - 80];        /* +80   */
	int (*init)(void);         /* +376  */
	u8 _pad2[1536 - 384];      /* +384  */
	void (*exit)(void);        /* +1536 */
	u8 _pad3[1664 - 1544];     /* +1544 */
#elif WALT_KVER >= 612
	u8 _pad1[392 - 80];        /* +80   */
	int (*init)(void);         /* +392  */
	u8 _pad2[1528 - 400];      /* +400  */
	void (*exit)(void);        /* +1528 */
	u8 _pad3[1600 - 1536];     /* +1536 */
#else
	u8 _pad1[392 - 80];        /* +80   */
	int (*init)(void);         /* +392  */
	u8 _pad2[1464 - 400];      /* +400  */
	void (*exit)(void);        /* +1464 */
	u8 _pad3[1536 - 1472];     /* +1472 */
#endif
};
#if WALT_KVER >= 618
_Static_assert(sizeof(struct plk_module) == 1664, "module size 1664");
_Static_assert(__builtin_offsetof(struct plk_module, init) == 376, "init@376");
_Static_assert(__builtin_offsetof(struct plk_module, exit) == 1536, "exit@1536");
#elif WALT_KVER >= 612
_Static_assert(sizeof(struct plk_module) == 1600, "module size 1600");
_Static_assert(__builtin_offsetof(struct plk_module, init) == 392, "init@392");
_Static_assert(__builtin_offsetof(struct plk_module, exit) == 1528, "exit@1528");
#else
_Static_assert(sizeof(struct plk_module) == 1536, "module size 1536");
_Static_assert(__builtin_offsetof(struct plk_module, init) == 392, "init@392");
_Static_assert(__builtin_offsetof(struct plk_module, exit) == 1464, "exit@1464");
#endif

struct modversion_info { unsigned long crc; char name[56]; };

#define PLK_INFO_NAME(tag, n) __plk_info_##tag##_##n
#define PLK_INFO_NAME_X(tag, n) PLK_INFO_NAME(tag, n)
#define PLK_MODULE_INFO(tag, info) \
	static const char __used __section(".modinfo") __aligned(1) \
	PLK_INFO_NAME_X(tag, __COUNTER__)[] = #tag "=" info

#define PLK_PLT_PAD \
	__used __section(".plt") static const u8 __plk_plt[0x400]; \
	__used __section(".init.plt") static const u8 __plk_iplt[0x200]

#define PAGE_SIZE	4096UL
extern void *memcpy(void *dst, const void *src, size_t n);
extern void *memset(void *dst, int c, size_t n);
extern size_t strlen(const char *s);
extern char *strsep(char **stringp, const char *delim);
extern int kstrtouint(const char *s, unsigned int base, unsigned int *res);
extern int scnprintf(char *buf, size_t size, const char *fmt, ...);

/* ---- cpufreq_policy shadow (6.12 GKI standard): cpu@0x1c, cpuinfo@0x28
 * (max_freq@0x28), governor@0x50, governor_data@0x58. ---- */
struct cpufreq_cpuinfo {
	unsigned int max_freq;       /* +0x28 */
	unsigned int min_freq;
	unsigned int transition_latency;
};
struct cpufreq_governor {
	char name[16];                   /* +0 */
};
struct cpufreq_policy {
	u8 _pad0[0x1c];                  /* +0    */
	unsigned int cpu;                /* +0x1c */
	u8 _pad1[0x28 - 0x20];           /* +0x20 */
	struct cpufreq_cpuinfo cpuinfo;  /* +0x28 */
	u8 _pad2[0x50 - 0x34];           /* +0x34 */
	struct cpufreq_governor *governor; /* +0x50 */
	void *governor_data;             /* +0x58 */
};
_Static_assert(__builtin_offsetof(struct cpufreq_policy, cpu) == 0x1c, "cpu@1c");
_Static_assert(__builtin_offsetof(struct cpufreq_policy, cpuinfo) == 0x28, "cpuinfo@28");
_Static_assert(__builtin_offsetof(struct cpufreq_policy, governor) == 0x50, "gov@50");
_Static_assert(__builtin_offsetof(struct cpufreq_policy, governor_data) == 0x58, "govdata@58");

extern struct cpufreq_policy *cpufreq_cpu_get(unsigned int cpu);
extern void cpufreq_cpu_put(struct cpufreq_policy *policy);

/* ---- sysfs attribute ABI (kobj_attribute: show@0x10, store@0x18).
 * WALT's custom sysfs_ops invokes these as (tunables, buf[, count]). ---- */
struct kobject;
struct attribute {
	const char *name;          /* +0 */
	u16 mode;                  /* +8 */
};
struct kobj_attribute {
	struct attribute attr;                       /* +0  (16) */
	ssize_t (*show)(void *t, char *buf);         /* +0x10 */
	ssize_t (*store)(void *t, const char *buf, size_t count); /* +0x18 */
};
_Static_assert(__builtin_offsetof(struct kobj_attribute, show) == 0x10, "show@10");
_Static_assert(__builtin_offsetof(struct kobj_attribute, store) == 0x18, "store@18");

struct attribute_group {
	const char *name;                  /* +0    */
	void *is_visible;                  /* +8    */
	void *is_bin_visible;              /* +0x10 */
#if WALT_KVER >= 618
	void *bin_size;                    /* +0x18 (6.18 adds this) */
	const struct attribute **attrs;    /* +0x20 */
	const void *bin_attrs;             /* +0x28 */
#else
	const struct attribute **attrs;    /* +0x18 */
	const void *bin_attrs;             /* +0x20 */
#endif
};
#if WALT_KVER >= 618
_Static_assert(__builtin_offsetof(struct attribute_group, attrs) == 0x20, "attrs@20");
#else
_Static_assert(__builtin_offsetof(struct attribute_group, attrs) == 0x18, "attrs@18");
#endif
extern int sysfs_create_group(struct kobject *kobj, const struct attribute_group *grp);
extern void sysfs_remove_group(struct kobject *kobj, const struct attribute_group *grp);

/* ---- kprobe shadow (6.12 GKI: size=128, addr@40, symbol_name@48).
 * Used to resolve a local symbol's address at init. ---- */
struct kprobe {
	void *hlist_next, *hlist_pprev;   /* +0  */
	void *list_next, *list_prev;      /* +16 */
	unsigned long nmissed;            /* +32 */
	void *addr;                       /* +40 */
	const char *symbol_name;          /* +48 */
#if WALT_KVER >= 618
	u8 _opaque[120 - 56];             /* +56 -> 120 (6.18) */
#else
	u8 _opaque[128 - 56];             /* +56 -> 128 (6.12) */
#endif
};
#if WALT_KVER >= 618
_Static_assert(sizeof(struct kprobe) == 120, "kprobe size 120");
#else
_Static_assert(sizeof(struct kprobe) == 128, "kprobe size 128");
#endif
_Static_assert(__builtin_offsetof(struct kprobe, addr) == 40, "addr@40");
_Static_assert(__builtin_offsetof(struct kprobe, symbol_name) == 48, "sym@48");
extern int register_kprobe(struct kprobe *p);
extern void unregister_kprobe(struct kprobe *p);

/* ---- module parameter ABI (6.12 GKI: kernel_param=40, kernel_param_ops=32) ---- */
struct kernel_param_ops {
	u32 flags;
	void *set, *get, *free;
};
_Static_assert(sizeof(struct kernel_param_ops) == 32, "kernel_param_ops size");
struct kernel_param {
	const char *name;       /* +0  */
	void *mod;              /* +8  */
	const void *ops;        /* +16 */
	unsigned short perm;    /* +24 */
	s8 level;               /* +26 */
	u8 flags;               /* +27 */
	u8 _pad[4];             /* +28 */
	void *arg;              /* +32 */
};
_Static_assert(sizeof(struct kernel_param) == 40, "kernel_param size");
extern const struct kernel_param_ops param_ops_uint;

#endif /* WALT_KERN_H */