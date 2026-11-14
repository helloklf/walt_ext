#!/bin/bash
# Build the read-only walt_diag offset diagnostic for a device.
# usage: wsl -d Ubuntu-24.04 bash common/build_diag.sh <device/kernel dir>
set -euo pipefail
COMMON="$(cd "$(dirname "$0")" && pwd)"
if [ $# -lt 1 ]; then echo "usage: build_diag.sh <device/kernel dir>"; exit 1; fi
DEVDIR="$(cd "$1" && pwd)"
source "$DEVDIR/config.sh"
KVER="${KVER:-612}"
BRANCH_PROT="${BRANCH_PROT:-pac-ret}"
NDK="$HOME/android-ndk-r26d/toolchains/llvm/prebuilt/linux-x86_64/bin"
CLANG="${CLANG:-/usr/bin/clang}"
OUT="$DEVDIR/out"; mkdir -p "$OUT"
"$CLANG" --target=aarch64-linux-gnu -std=gnu11 -O2 \
  -D__KERNEL__ -DMODULE -DKBUILD_MODNAME='"walt_diag"' -DKBUILD_BASENAME='"walt_diag"' \
  -DWALT_KVER="$KVER" \
  -I"$COMMON" -I"$DEVDIR" \
  -fno-pic -fno-PIE -fno-common -fno-builtin -fno-stack-protector \
  -fno-asynchronous-unwind-tables -fno-unwind-tables -fno-delete-null-pointer-checks \
  -fno-strict-overflow -fno-optimize-sibling-calls -fno-omit-frame-pointer -ffixed-x18 \
  -fsanitize=kcfi -fsanitize-cfi-icall-experimental-normalize-integers \
  -mbranch-protection="$BRANCH_PROT" -mgeneral-regs-only -mstrict-align -mno-outline-atomics \
  -c "$COMMON/walt_diag.c" -o "$OUT/walt_diag.o"
"$NDK/ld.lld" -r -m aarch64elf -z noexecstack --build-id=sha1 \
  -T "$COMMON/module.lds" -o "$OUT/walt_diag.ko" "$OUT/walt_diag.o"

# Patch init/exit kCFI preambles to this kernel's expected hashes (the kCFI scheme
# differs across GKI generations; without this the module crashes on insmod).
python3 - "$OUT/walt_diag.ko" "$KCFI_INIT" "$KCFI_EXIT" <<'PY'
import struct, sys
ko, kinit, kexit = sys.argv[1:4]
d = bytearray(open(ko,'rb').read())
shoff=struct.unpack_from('<Q',d,0x28)[0]; shent=struct.unpack_from('<H',d,0x3a)[0]
shnum=struct.unpack_from('<H',d,0x3c)[0]; shstr=struct.unpack_from('<H',d,0x3e)[0]
secs=[list(struct.unpack_from('<IIQQQQIIQQ',d,shoff+i*shent)) for i in range(shnum)]
st=secs[shstr]; stb=d[st[4]:st[4]+st[5]]
for s in secs: s.append(stb[s[0]:stb.find(b'\0',s[0])].decode())
sym=next(s for s in secs if s[10]=='.symtab'); strs=secs[sym[6]]
strt=d[strs[4]:strs[4]+strs[5]]
patch={'init_module':int(kinit,16), 'cleanup_module':int(kexit,16)}
for i in range(sym[5]//24):
    o=sym[4]+i*24
    nm,info,other,shndx,val,size=struct.unpack_from('<IBBHQQ',d,o)
    name=strt[nm:strt.find(b'\0',nm)].decode()
    if name in patch:
        sec=secs[shndx]; struct.pack_into('<I',d,sec[4]+val-4,patch[name])
open(ko,'wb').write(d)
print("patched init/exit kCFI preambles")
PY
echo "built $OUT/walt_diag.ko"