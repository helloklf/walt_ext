#!/usr/bin/env python3
"""Dump struct member byte-offsets from a BTF blob (e.g. /sys/kernel/btf/vmlinux).
usage: btf_offsets.py <btf> <struct> [struct ...]"""
import struct, sys

def parse(path):
    d = open(path, 'rb').read()
    magic, ver, flags, hdr_len, type_off, type_len, str_off, str_len = struct.unpack_from('<HBBIIIII', d, 0)
    assert magic == 0xeB9F, hex(magic)
    types = d[hdr_len + type_off: hdr_len + type_off + type_len]
    strs = d[hdr_len + str_off: hdr_len + str_off + str_len]
    def s(o): return strs[o:strs.find(b'\0', o)].decode('utf-8', 'replace')
    out = []
    p = 0
    while p < len(types):
        name_off, info, sz = struct.unpack_from('<III', types, p)
        kind = (info >> 24) & 0x1f
        vlen = info & 0xffffff
        base = p + 12
        if kind in (4, 5):  # STRUCT/UNION
            members = []
            for i in range(vlen):
                mn, mt, mo = struct.unpack_from('<III', types, base + i*12)
                members.append((s(mn), mt, mo // 8))
            out.append((s(name_off), kind, sz, members))
            p = base + vlen*12
        elif kind == 1:  # INT
            p = base + 4
        elif kind == 3:  # ARRAY
            p = base + 12
        elif kind == 6:  # ENUM
            p = base + vlen*8
        elif kind == 13:  # FUNC_PROTO
            p = base + vlen*8
        elif kind == 14:  # VAR
            p = base + 4
        elif kind == 15:  # DATASEC
            p = base + vlen*12
        elif kind == 19:  # ENUM64
            p = base + vlen*12
        else:
            p = base
    return out

def main():
    structs = parse(sys.argv[1])
    want = sys.argv[2:]
    byname = {}
    for name, kind, sz, members in structs:
        if name:
            byname.setdefault(name, (sz, members))
    for w in want:
        if w not in byname:
            print(f"struct {w}: NOT FOUND"); continue
        sz, members = byname[w]
        print(f"struct {w} {{  // size={sz}")
        for mn, mt, mo in members:
            print(f"  +0x{mo:03x}  {mn}")
        print("}")

if __name__ == '__main__':
    main()