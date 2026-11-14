#!/usr/bin/env python3
"""Parse Android vendor_boot v4 header + ramdisk table; optionally extract the
platform ramdisk fragment. usage: parse_vboot.py <vb.img> [extract_out.cpio]"""
import struct, sys

d = open(sys.argv[1], 'rb').read()
magic = d[0:8]
print("magic:", magic)
(ver, page, kaddr, raddr, vrd_size) = struct.unpack_from('<IIIII', d, 8)
print(f"header_version={ver} page_size={page} vendor_ramdisk_size={vrd_size}")
cmdline = d[28:28+2048].split(b'\0')[0].decode('utf-8','replace')
print("cmdline:", cmdline[:120])
name = d[2080:2096].split(b'\0')[0].decode('utf-8','replace')
(header_size, dtb_size) = struct.unpack_from('<II', d, 2096)
dtb_addr = struct.unpack_from('<Q', d, 2104)[0]
(tbl_size, tbl_num, tbl_entry_size, bc_size) = struct.unpack_from('<IIII', d, 2112)
print(f"name={name!r} header_size={header_size} dtb_size={dtb_size}")
print(f"ramdisk_table: size={tbl_size} num={tbl_num} entry_size={tbl_entry_size} bootconfig={bc_size}")

def align(x, a): return (x + a - 1) // a * a
rd_off = page
dtb_off = align(rd_off + vrd_size, page)
tbl_off = align(dtb_off + dtb_size, page)
bc_off = align(tbl_off + tbl_size, page)
print(f"vendor_ramdisk@{rd_off} dtb@{dtb_off} table@{tbl_off} bootconfig@{bc_off}")

print("=== ramdisk table entries ===")
frags = []
for i in range(tbl_num):
    o = tbl_off + i * tbl_entry_size
    rsz, roff, rtype = struct.unpack_from('<III', d, o)
    rname = d[o+12:o+44].split(b'\0')[0].decode('utf-8','replace')
    print(f"  [{i}] name={rname!r} type={rtype} size={rsz} offset={roff}")
    frags.append((rname, rtype, rsz, roff))

if len(sys.argv) > 2:
    # extract platform (type 1) fragments concatenated
    out = bytearray()
    for rname, rtype, rsz, roff in frags:
        if rtype == 1:  # PLATFORM
            out += d[rd_off + roff: rd_off + roff + rsz]
    open(sys.argv[2], 'wb').write(out)
    print(f"extracted platform ramdisk -> {sys.argv[2]} ({len(out)} bytes)")
    print("first bytes:", out[:8].hex())