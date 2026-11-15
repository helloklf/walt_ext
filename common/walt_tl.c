// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * walt_tl — per-frequency-band target_loads for the WALT cpufreq governor.
 * Generic logic; device/vendor specifics come from device.h / crcs.h / config.sh
 * in the per-device <device>/<kernel>/ directory.
 *
 * The WALT governor (sched-walt.ko) has NO "target_loads" tunable; its band
 * mechanism is "zone_max_util_pct" (space-separated "freq pct" pairs). Each
 * zone's pct IS a target load: update_util_inflate_factor() turns it into a util
 * inflation factor (100/pct) used by the freq decision.
 *
 * This module adds a "target_loads" sysfs file to each WALT policy accepting the
 * interactive-style format:
 *     80 1200000:85 1900000:90
 * (default load 80; >=1200000 use 85; >=1900000 use 90). It translates that into
 * WALT zone pairs and calls the ORIGINAL vendor zone store function — reusing the
 * vendor parser/validator/locking/cluster-iteration/inflate computation verbatim.
 * The freq-decision hot path is NOT modified.
 *
 * Translation (interactive "from freq upward" -> WALT "upper-bound" zones):
 *     80 1200000:85 1900000:90  ->  zones (1200000,80) (1900000,85) (max_freq,90)
 *
 * Safety: only cold-path sysfs is added; on exit the groups are removed.
 * Switching the governor away from WALT destroys the tunables kobject, which
 * auto-removes these files (no dangling). Reload the module after switching back.
 *
 * Provenance: the target_loads file keeps its canonical name/location (vendor
 * perf daemons write it). Module provenance is identified via
 * /sys/module/walt_tl/version (auto-created by the module core from the
 * modinfo version tag) — no marker files are added to the vendor tunables
 * kobject at all; an early named-group marker there proved fatal on 6.6.
 */
#include "walt_kern.h"
#include "device.h"

#define TAG "walt_tl"
PLK_PLT_PAD;

extern struct plk_module wt_this_module __asm__("__this_module");

/* resolved at init: original vendor zone store(tunables, buf, count) */
typedef ssize_t (*zone_store_fn)(void *t, const char *buf, size_t count);
static zone_store_fn zone_store;

/* Some vendors (android15/6.6) guard zone_store with a per-cluster "write once"
 * flag array: once a cluster is initialized, zone_store skips rewriting its
 * zones, so repeated writes are no-ops. For those, resolve the flag array via
 * kallsyms_lookup_name and clear it before each zone_store call. */
#ifdef TL_ZONE_GUARD_SYM
typedef unsigned long (*lookup_fn)(const char *name);
static lookup_fn klookup;
static unsigned int *zone_guard;

__attribute__((no_sanitize("kcfi")))
static unsigned long call_lookup(const char *name)
{
	return klookup(name);
}
#endif

#define MAX_POL 8
struct tl_rec {
	void *tunables;
	void *kobj;
	unsigned int max_freq;
	int active;
};
static struct tl_rec recs[MAX_POL];
static int nrecs;

/* ---- tunables accessors (offsets from device.h) ---- */
static void *policy_tunables(struct cpufreq_policy *pol)
{
	void *gd = pol->governor_data;
	if (!gd)
		return NULL;
	return *(void **)((char *)gd + TL_GD_TUNABLES_OFF);
}
static void *tunables_kobj(void *t) { return (char *)t + TL_KOBJ_OFF; }
static u32 *tunables_zones(void *t) { return (u32 *)((char *)t + TL_ZONES_OFF); }

static unsigned int lookup_maxfreq(void *t)
{
	int i;
	for (i = 0; i < nrecs; i++)
		if (recs[i].active && recs[i].tunables == t)
			return recs[i].max_freq;
	return FALLBACK_MAX_FREQ;
}

static int streq(const char *a, const char *b)
{
	while (*a && *a == *b) { a++; b++; }
	return *a == *b;
}
static int is_walt(struct cpufreq_policy *pol)
{
	return pol->governor && streq(pol->governor->name, GOV_NAME);
}

/* ---- target_loads show: zones -> "default freq:load freq:load ..." ---- */
static ssize_t tl_show(void *t, char *buf)
{
	u32 *z = tunables_zones(t);
	int n = 0, i, len;

	while (n < TL_MAX_ZONES && z[n * 2] != 0xffffffffu)
		n++;
	if (n == 0)
		return scnprintf(buf, PAGE_SIZE, "0\n");

#ifdef TL_ZONES_DESCENDING
	/* stored high->low: default = lowest zone pct, bands emitted ascending */
	len = scnprintf(buf, PAGE_SIZE, "%u", z[(n - 1) * 2 + 1]);
	for (i = 0; i < n - 1; i++)
		len += scnprintf(buf + len, PAGE_SIZE - len, " %u:%u",
				 z[(n - 1 - i) * 2], z[(n - 2 - i) * 2 + 1]);
#else
	/* stored low->high: default = zone0.pct, band i = zone[i].freq:zone[i+1].pct */
	len = scnprintf(buf, PAGE_SIZE, "%u", z[1]);
	for (i = 0; i < n - 1; i++)
		len += scnprintf(buf + len, PAGE_SIZE - len, " %u:%u",
				 z[i * 2], z[i * 2 + 3]);
#endif
	len += scnprintf(buf + len, PAGE_SIZE - len, "\n");
	return len;
}

/* Indirect call into the vendor zone store. Its kCFI preamble encodes the
 * vendor's internal struct signature, which a shadow-struct function pointer
 * cannot reproduce; disable the caller-side kCFI check for this one indirect
 * call (the target is a real, correctly-tagged function). */
__attribute__((no_sanitize("kcfi")))
static ssize_t invoke_zone_store(void *t, const char *buf, size_t n)
{
	return zone_store(t, buf, n);
}

/* ---- target_loads store: "default freq:load ..." -> WALT zone pairs ---- */
static ssize_t tl_store(void *t, const char *buf, size_t count)
{
	char tmp[512];
	char zbuf[512];
	char *p, *tok, *colon;
	unsigned int defload = 0, max_freq, f, l;
	struct { unsigned int freq, load; } bands[TL_MAX_ZONES];
	int nb = 0, i, zlen;
	ssize_t ret;

	if (!zone_store)
		return -95; /* EOPNOTSUPP */
	if (count == 0 || count >= sizeof(tmp))
		return -22; /* EINVAL */
	memcpy(tmp, buf, count);
	tmp[count] = '\0';
	max_freq = lookup_maxfreq(t);

	p = tmp;
	/* first non-empty token = default load */
	do { tok = strsep(&p, " \t\r\n"); } while (tok && !*tok);
	if (!tok)
		return -22;
	if (kstrtouint(tok, 10, &defload))
		return -22;

	/* remaining tokens = freq:load */
	while ((tok = strsep(&p, " \t\r\n")) != NULL) {
		if (!*tok)
			continue;
		colon = tok;
		while (*colon && *colon != ':')
			colon++;
		if (*colon != ':')
			return -22;
		*colon = '\0';
		if (kstrtouint(tok, 10, &f) || kstrtouint(colon + 1, 10, &l))
			return -22;
		if (nb >= TL_MAX_ZONES)
			return -22;
		bands[nb].freq = f;
		bands[nb].load = l;
		nb++;
	}

	/* Build WALT zone pairs: (bands[0].freq, defload), (bands[i].freq,
	 * bands[i-1].load), ..., (max_freq, bands[nb-1].load). Some vendors
	 * (e.g. android15/6.6) require the input frequencies in descending order
	 * and reverse them internally; define TL_ZONES_DESCENDING for those. */
	{
		unsigned int zf[TL_MAX_ZONES + 1], zp[TL_MAX_ZONES + 1];
		int nz, j;
		if (nb == 0) {
			zf[0] = max_freq; zp[0] = defload; nz = 1;
		} else {
			zf[0] = bands[0].freq; zp[0] = defload;
			for (i = 1; i < nb; i++) {
				zf[i] = bands[i].freq; zp[i] = bands[i - 1].load;
			}
			zf[nb] = max_freq; zp[nb] = bands[nb - 1].load;
			nz = nb + 1;
		}
		zlen = 0;
		for (j = 0; j < nz; j++) {
#ifdef TL_ZONES_DESCENDING
			int k = nz - 1 - j;
#else
			int k = j;
#endif
			zlen += scnprintf(zbuf + zlen, sizeof(zbuf) - zlen,
					  j ? " %u %u" : "%u %u", zf[k], zp[k]);
		}
	}

#ifdef TL_ZONE_GUARD_SYM
	/* Clear the write-once guard so zone_store rewrites the zones, and reset
	 * the zone array to the -1 terminator: this vendor's zone_store neither
	 * clears stale zones beyond the new count nor rewrites the terminator. */
	if (zone_guard) {
		int g;
		for (g = 0; g < TL_ZONE_GUARD_COUNT; g++)
			zone_guard[g] = 0;
	}
	memset(tunables_zones(t), 0xff, TL_MAX_ZONES * 8);
#endif
	ret = invoke_zone_store(t, zbuf, (size_t)zlen);
	if (ret < 0)
		return ret;
	return count;
}

static struct kobj_attribute tl_attr = {
	.attr	= { .name = "target_loads", .mode = 0644 },
	.show	= tl_show,
	.store	= tl_store,
};
static const struct attribute *tl_attrs[] = { &tl_attr.attr, NULL };
static const struct attribute_group tl_group = {
	.name	= NULL,
	.attrs	= tl_attrs,
};

/* Provenance: NOT a sysfs marker in the vendor tunables dir. The module core
 * auto-creates /sys/module/walt_tl/version from the modinfo "version=" tag
 * (added below), and /sys/module/walt_tl/parameters/selftest from the module
 * param — both live in module-owned space with the standard module ktype.
 * HARD-WON LESSON: any extra file we add to the vendor tunables kobject goes
 * through the vendor's custom sysfs_ops. A named-group marker ("walt_tl/version"
 * subdirectory) killed the kernel on 6.6 at read time (the named-group dir's
 * kernfs priv is not the vendor kobject there, so show() got a bogus tunables
 * pointer). We now add NOTHING beyond target_loads itself; users verify
 * provenance via /sys/module/walt_tl/version alongside the target_loads file. */
#define TL_VERSION "1.3"

/* ---- resolve a symbol's address via a transient kprobe (text symbols) ---- */
static void *resolve_sym(const char *name)
{
	struct kprobe kp;
	void *addr;

	memset(&kp, 0, sizeof(kp));
	kp.symbol_name = name;
	if (register_kprobe(&kp)) {
		pr_err("[%s] register_kprobe(%s) failed\n", TAG, name);
		return NULL;
	}
	addr = kp.addr;
	unregister_kprobe(&kp);
	return addr;
}

static int resolve_zone_store(void)
{
	zone_store = (zone_store_fn)resolve_sym(ZONE_STORE_SYM);
	if (!zone_store) {
		pr_err("[%s] %s address is NULL\n", TAG, ZONE_STORE_SYM);
		return -2; /* ENOENT */
	}
#ifdef TL_ZONE_GUARD_SYM
	klookup = (lookup_fn)resolve_sym("kallsyms_lookup_name");
	if (!klookup) {
		pr_err("[%s] kallsyms_lookup_name not found\n", TAG);
		return -2;
	}
	zone_guard = (unsigned int *)call_lookup(TL_ZONE_GUARD_SYM);
	if (!zone_guard) {
		pr_err("[%s] %s not found\n", TAG, TL_ZONE_GUARD_SYM);
		return -2;
	}
	pr_info("[%s] write-once guard at %px (will clear before writes)\n",
		TAG, zone_guard);
#endif
	return 0;
}

static int already_recorded(void *t)
{
	int i;
	for (i = 0; i < nrecs; i++)
		if (recs[i].tunables == t)
			return 1;
	return 0;
}

/* ---- kernel-side selftest: exercise tl_store without userspace sysfs write
 * (bypasses SELinux). Triggered by insmod selftest=1. Restores baseline after. ---- */
static unsigned int selftest;
/* Selftest frequencies must be valid OPPs on the target device (the vendor
 * zone_store rejects non-OPP frequencies). Override SELFTEST_CFG in device.h. */
#ifndef SELFTEST_CFG
#define SELFTEST_CFG "80 1200000:85 1900000:90"
#endif

static void run_selftest(void)
{
	static const char cfg[] = SELFTEST_CFG;
	static const char restore[] = SELFTEST_RESTORE;
	void *t = recs[0].tunables;
	u32 *z = tunables_zones(t);
	char buf[256];
	ssize_t r;

	pr_info("[%s] SELFTEST tunables=%px\n", TAG, t);
	tl_show(t, buf);
	pr_info("[%s] before: %s", TAG, buf);
#ifdef SELFTEST_PROBE
	{
		static const char *probes[] = {
			"1363200 80",
			"1363200 80 1996800 85",
			"1996800 85 1363200 80",
			"1363200 80 1996800 85 3532800 90",
			"1363200 80 1996800 85 2438400 90",
			"3532800 90 1996800 85 1363200 80",
		};
		int i;
		for (i = 0; i < 6; i++) {
			r = invoke_zone_store(t, probes[i], strlen(probes[i]));
			pr_info("[%s] probe[%d] \"%s\" ret=%zd\n", TAG, i, probes[i], r);
		}
		invoke_zone_store(t, restore, sizeof(restore) - 1);
	}
#endif
	r = tl_store(t, cfg, sizeof(cfg) - 1);
	pr_info("[%s] tl_store(\"%s\") ret=%zd\n", TAG, cfg, r);
	pr_info("[%s] zones: %u:%u %u:%u %u:%u %u:%u\n", TAG,
		z[0], z[1], z[2], z[3], z[4], z[5], z[6], z[7]);
	tl_show(t, buf);
	pr_info("[%s] after: %s", TAG, buf);
	r = invoke_zone_store(t, restore, sizeof(restore) - 1);
	pr_info("[%s] restore ret=%zd\n", TAG, r);
	tl_show(t, buf);
	pr_info("[%s] restored: %s", TAG, buf);
}

/* ================= module lifecycle ================= */
extern int init_module(void);
extern void cleanup_module(void);

int init_module(void)
{
	int ret, cpu;

	ret = resolve_zone_store();
	if (ret)
		return ret;

	for (cpu = 0; cpu < MAX_POL; cpu++) {
		struct cpufreq_policy *pol = cpufreq_cpu_get(cpu);
		void *t;
		if (!pol)
			continue;
		t = policy_tunables(pol);
		if (t && is_walt(pol) && !already_recorded(t) && nrecs < MAX_POL) {
			void *kobj = tunables_kobj(t);
			unsigned int mf = pol->cpuinfo.max_freq;

			ret = sysfs_create_group((struct kobject *)kobj, &tl_group);
			if (ret) {
				pr_err("[%s] sysfs_create_group cpu%d failed: %d\n",
				       TAG, cpu, ret);
			} else {
				recs[nrecs].tunables = t;
				recs[nrecs].kobj = kobj;
				recs[nrecs].max_freq = mf;
				recs[nrecs].active = 1;
				nrecs++;
				pr_info("[%s] target_loads added: cpu%d max_freq=%u\n",
					TAG, cpu, mf);
			}
		}
		cpufreq_cpu_put(pol);
	}

	if (nrecs == 0) {
		pr_err("[%s] no active %s policy found\n", TAG, GOV_NAME);
		return -19; /* ENODEV */
	}
	pr_info("[%s] loaded: %d policy target_loads, zone_store=%px\n",
		TAG, nrecs, zone_store);
	if (selftest)
		run_selftest();
	return 0;
}

void cleanup_module(void)
{
	int i;
	for (i = 0; i < nrecs; i++) {
		if (recs[i].active) {
			sysfs_remove_group((struct kobject *)recs[i].kobj, &tl_group);
			recs[i].active = 0;
		}
	}
	pr_info("[%s] unloaded: removed target_loads from %d policies\n", TAG, nrecs);
}

PLK_MODULE_INFO(description, "Per-frequency-band target_loads for " DEVICE_DESC);
PLK_MODULE_INFO(license, "GPL");
PLK_MODULE_INFO(name, "walt_tl");
PLK_MODULE_INFO(version, TL_VERSION);
PLK_MODULE_INFO(depends, "");
PLK_MODULE_INFO(vermagic, PLK_VERMAGIC);
PLK_MODULE_INFO(parmtype, "selftest:uint");
PLK_MODULE_INFO(parm, "selftest:run kernel-side store selftest at insmod (1=yes)");

__visible struct plk_module wt_this_module
__asm__("__this_module")
__section(".gnu.linkonce.this_module") = {
	.name = "walt_tl",
	.init = init_module,
	.exit = cleanup_module,
};

static const struct kernel_param __used __section("__param") __aligned(8)
__wt_param_selftest = {
	.name	= "selftest",
	.mod	= &wt_this_module,
	.ops	= &param_ops_uint,
	.perm	= 0644,
	.arg	= &selftest,
};

/* device-specific imported-symbol CRCs */
#include "crcs.h"