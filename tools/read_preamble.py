#!/usr/bin/env python3
"""Read the kCFI preamble (the 4-byte type hash immediately before a function)
from a kernel module. Use it to harvest KCFI_SHOW / KCFI_STORE for config.sh:
the vendor WALT sysfs_ops checks attr->show/store against these hashes.

usage: python3 read_preamble.py <module.ko> <func> [func ...]
  e.g. python3 read_preamble.py sched-walt.ko zone_max_util_pct_show zone_max_util_pct_store
"""
import struct, sys

def sections(d):
    shoff = struct.unpack_from('<Q', d, 0x28)[0]
    shent = struct.unpack_from('<H', d, 0x3a)[0]
    shnum = struct.unpack_from('<H', d, 0x3c)[0]
    shstr = struct.unpack_from('<H', d, 0x3e)[0]
    secs = [list(struct.unpack_from('<IIQQQQIIQQ', d, shoff + i*shent)) for i in range(shnum)]
    st = secs[shstr]; stb = d[st[4]:st[4]+st[5]]
    for s in secs:
        s.append(stb[s[0]:stb.find(b'\0', s[0])].decode())
    return secs

def main():
    if len(sys.argv) < 3:
        print(__doc__); sys.exit(1)
    d = open(sys.argv[1], 'rb').read()
    secs = sections(d)
    sym = next(s for s in secs if s[10] == '.symtab')
    strs = secs[sym[6]]                       # sh_link -> .strtab (index 6!)
    strt = d[strs[4]:strs[4]+strs[5]]
    syms = {}
    for i in range(sym[5]//24):
        o = sym[4] + i*24
        nm, info, other, shndx, val, size = struct.unpack_from('<IBBHQQ', d, o)
        name = strt[nm:strt.find(b'\0', nm)].decode()
        if (info & 0xf) == 2 and name:        # STT_FUNC
            syms[name] = (shndx, val)
    for fn in sys.argv[2:]:
        if fn not in syms:
            print(f"{fn:32} NOT FOUND"); continue
        shndx, val = syms[fn]
        foff = secs[shndx][4] + val
        h = struct.unpack_from('<I', d, foff-4)[0]
        print(f"{fn:32} preamble = {hex(h)}")

if __name__ == '__main__':
    main()