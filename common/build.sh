#!/bin/bash
# Generic raw-clang build for walt_tl. 6.12 GKI rules: kCFI normalize-integers,
# branch protection pac-ret (NO BTI). Device specifics come from <devdir>/config.sh
# (MODNAME, KCFI_INIT/SHOW/STORE) and <devdir>/{device.h,crcs.h}.
#
# usage: wsl -d Ubuntu-24.04 bash common/build.sh <device/kernel dir>
#   e.g. bash common/build.sh plk110/6.12.69
set -euo pipefail
COMMON="$(cd "$(dirname "$0")" && pwd)"
if [ $# -lt 1 ]; then echo "usage: build.sh <device/kernel dir>"; exit 1; fi
DEVDIR="$(cd "$1" && pwd)"
# shellcheck disable=SC1091
source "$DEVDIR/config.sh"   # MODNAME, KVER, BRANCH_PROT, KCFI_INIT, KCFI_SHOW, KCFI_STORE
KVER="${KVER:-612}"
BRANCH_PROT="${BRANCH_PROT:-pac-ret}"

NDK="$HOME/android-ndk-r26d/toolchains/llvm/prebuilt/linux-x86_64/bin"
CLANG="${CLANG:-/usr/bin/clang}"
LLD="${LLD:-$NDK/ld.lld}"
OUT="$DEVDIR/out"
mkdir -p "$OUT"

echo "=== toolchain: $CLANG ==="
"$CLANG" --version | head -1
echo "=== device: $DEVDIR (MODNAME=$MODNAME) ==="

"$CLANG" --target=aarch64-linux-gnu -std=gnu11 -O2 \
  -D__KERNEL__ -DMODULE -DKBUILD_MODNAME="\"$MODNAME\"" -DKBUILD_BASENAME="\"$MODNAME\"" \
  -DWALT_KVER="$KVER" ${PROBE:+-DSELFTEST_PROBE} \
  -I"$COMMON" -I"$DEVDIR" \
  -fno-pic -fno-PIE -fno-common -fno-builtin -fno-stack-protector \
  -fno-asynchronous-unwind-tables -fno-unwind-tables \
  -fno-delete-null-pointer-checks -fno-strict-overflow \
  -fno-optimize-sibling-calls -fno-omit-frame-pointer -ffixed-x18 \
  -fsanitize=kcfi -fsanitize-cfi-icall-experimental-normalize-integers \
  -mbranch-protection="$BRANCH_PROT" -mgeneral-regs-only -mstrict-align -mno-outline-atomics \
  -c "$COMMON/walt_tl.c" -o "$OUT/$MODNAME.o"

"$LLD" -r -m aarch64elf -z noexecstack --build-id=sha1 \
  -T "$COMMON/module.lds" -o "$OUT/$MODNAME.ko" "$OUT/$MODNAME.o"

echo "=== built $OUT/$MODNAME.ko ==="

# Binary-patch the kCFI preambles of every function the kernel calls indirectly
# (init_module, cleanup_module, tl_show, tl_store) to the hashes this kernel
# expects. The preamble is only a caller-side type tag; patching it does not change
# the function body. Needed because the kCFI type-hash scheme differs across GKI
# generations (android15/6.6 vs android16/6.12+), and the vendor sysfs_ops checks
# its own hashes for show/store.
python3 - "$OUT/$MODNAME.ko" "$KCFI_INIT" "$KCFI_EXIT" "$KCFI_SHOW" "$KCFI_STORE" <<'PY'
import struct, sys
ko, kinit, kexit, kshow, kstore = sys.argv[1:6]
d = bytearray(open(ko,'rb').read())
shoff=struct.unpack_from('<Q',d,0x28)[0]; shent=struct.unpack_from('<H',d,0x3a)[0]
shnum=struct.unpack_from('<H',d,0x3c)[0]; shstr=struct.unpack_from('<H',d,0x3e)[0]
secs=[list(struct.unpack_from('<IIQQQQIIQQ',d,shoff+i*shent)) for i in range(shnum)]
st=secs[shstr]; stb=d[st[4]:st[4]+st[5]]
for s in secs: s.append(stb[s[0]:stb.find(b'\0',s[0])].decode())
sym=next(s for s in secs if s[10]=='.symtab'); strs=secs[sym[6]]
strt=d[strs[4]:strs[4]+strs[5]]
expect={'init_module':int(kinit,16), 'cleanup_module':int(kexit,16),
        'tl_show':int(kshow,16), 'tl_store':int(kstore,16)}
patch=dict(expect)
found={}
for i in range(sym[5]//24):
    o=sym[4]+i*24
    nm,info,other,shndx,val,size=struct.unpack_from('<IBBHQQ',d,o)
    name=strt[nm:strt.find(b'\0',nm)].decode()
    if name in expect and name not in found:
        sec=secs[shndx]; foff=sec[4]+val
        if name in patch: struct.pack_into('<I',d,foff-4,patch[name])
        found[name]=struct.unpack_from('<I',d,foff-4)[0]
open(ko,'wb').write(d)
bad=0
for name,exp in expect.items():
    if name in found:
        h=found[name]; ok=h==exp; bad+=(not ok)
        tag='(patched)' if name in patch else ''
        print(f"{name:14} CFI = {hex(h)}  (expect {hex(exp)})  {'OK' if ok else 'MISMATCH!'} {tag}")
    else:
        print(f"{name:14} NOT FOUND"); bad+=1
sys.exit(1 if bad else 0)
PY