#!/usr/bin/env python3
# 从 ko 的 EXTENDED_MODVERSIONS + 传统 __versions 提取 name->CRC
import struct, sys

fn = sys.argv[1] if len(sys.argv) > 1 else "sched-walt.ko"
d = open(fn, "rb").read()
shoff = struct.unpack_from('<Q', d, 0x28)[0]
shentsize = struct.unpack_from('<H', d, 0x3a)[0]
shnum = struct.unpack_from('<H', d, 0x3c)[0]
shstrndx = struct.unpack_from('<H', d, 0x3e)[0]
shs = [struct.unpack_from('<IIQQQQIIQQ', d, shoff + i*shentsize) for i in range(shnum)]
strtab_off = shs[shstrndx][4]
def nm(o):
    e = d.index(b'\0', strtab_off+o); return d[strtab_off+o:e].decode()
secs = {nm(s[0]): s for s in shs}

crcs = {}
# EXTENDED_MODVERSIONS
if '__version_ext_crcs' in secs and '__version_ext_names' in secs:
    cs = secs['__version_ext_crcs']; ns = secs['__version_ext_names']
    crc_data = d[cs[4]:cs[4]+cs[5]]
    name_data = d[ns[4]:ns[4]+ns[5]]
    ncrc = len(crc_data)//4
    names = name_data.split(b'\0')
    for i in range(ncrc):
        crc = struct.unpack_from('<I', crc_data, i*4)[0]
        name = names[i].decode() if i < len(names) else f"?{i}"
        crcs[name] = crc
# 传统 __versions: unsigned long crc(8) + name@+8
if '__versions' in secs:
    vs = secs['__versions']
    vd = d[vs[4]:vs[4]+vs[5]]
    for off in range(0, len(vd) - 63, 64):
        crc = struct.unpack_from('<Q', vd, off)[0]
        name = vd[off+8:off+64].split(b'\0')[0].decode('utf-8', 'replace')
        if name and name not in crcs:
            crcs[name] = crc & 0xffffffff

print(f"# {fn}: {len(crcs)} CRCs")
# 重点输出我需要的
want = ['sysfs_create_file', 'sysfs_remove_file', 'sysfs_create_group', 'sysfs_remove_group',
        'kallsyms_lookup_name', 'register_kprobe', 'unregister_kprobe', 'register_kretprobe',
        'unregister_kretprobe', 'kobject_get', 'kobject_put', 'kobject_create_and_add',
        'printk', '_printk', 'sprintf', 'scnprintf', 'snprintf', 'strlen', 'strsep',
        'kstrtouint', 'kstrtoint', 'mutex_lock', 'mutex_unlock', 'module_layout',
        'kmalloc_caches', 'kmem_cache_alloc_trace', 'kfree', 'vzalloc', 'vfree',
        '__stack_chk_fail', 'memcpy', 'memset', 'cpufreq_register_governor']
for w in want:
    if w in crcs:
        print(f"{w} = 0x{crcs[w]:08x}")
print("# ---- 全部 ----")
for k in sorted(crcs):
    print(f"{k} = 0x{crcs[k]:08x}")