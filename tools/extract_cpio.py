#!/usr/bin/env python3
# 极简 cpio newc 解析器：从 vendor ramdisk 抠出 walt/cpufreq 相关 ko
import struct, sys, os

CPIO = sys.argv[1] if len(sys.argv) > 1 else "vendor_ramdisk.cpio"
OUT = sys.argv[2] if len(sys.argv) > 2 else "."
d = open(CPIO, "rb").read()
pos = 0
n = len(d)
found = []
allko = []

def align4(x):
    return (x + 3) & ~3

while pos + 110 <= n:
    magic = d[pos:pos+6]
    if magic not in (b"070701", b"070702"):
        # 尝试跳过填充
        break
    hdr = d[pos:pos+110]
    fields = [int(hdr[i:i+8], 16) for i in range(6, 110, 8)]
    (ino, mode, uid, gid, nlink, mtime, filesize,
     devmajor, devminor, rdevmajor, rdevminor, namesize, check) = fields
    name_start = pos + 110
    name = d[name_start:name_start+namesize-1].decode("utf-8", "replace")
    data_start = align4(name_start + namesize)
    data_end = data_start + filesize
    if name == "TRAILER!!!":
        break
    if name.endswith(".ko"):
        allko.append(name)
        base = os.path.basename(name)
        if any(k in base.lower() for k in ("walt", "cpufreq", "bouncing")):
            outpath = os.path.join(OUT, base)
            with open(outpath, "wb") as f:
                f.write(d[data_start:data_end])
            found.append((name, filesize, outpath))
    pos = align4(data_end)

print("=== 抠出的模块 ===")
for name, sz, out in found:
    print(f"{name}  {sz} bytes -> {out}")
print(f"\n=== ramdisk 内 ko 总数: {len(allko)} ===")
for k in allko:
    if any(x in k.lower() for x in ("walt", "cpufreq", "bouncing", "sched")):
        print("  ", k)