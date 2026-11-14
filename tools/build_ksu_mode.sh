#!/bin/bash
# Cross-compile ksu_mode.c (KSU jailbreak/late-load detector) for arm64 Android.
# usage: wsl -d Ubuntu-24.04 bash tools/build_ksu_mode.sh
#
# Preferred: system clang + NDK sysroot, dynamically linked against bionic (~8KB).
#   (The NDK wrapper clang itself may fail in some WSL setups; using the system
#    clang with the NDK sysroot/linker/compiler-rt avoids that.)
# Fallback: aarch64-linux-gnu-gcc -static (glibc, ~600KB stripped; runs on Android
#   because it is fully static).
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
OUT="$HERE/ksu_mode"
NDK="$HOME/android-ndk-r26d/toolchains/llvm/prebuilt/linux-x86_64"
SYSROOT="$NDK/sysroot"

if [ -x /usr/bin/clang ] && [ -d "$SYSROOT" ] && [ -x "$NDK/bin/ld.lld" ]; then
  /usr/bin/clang --target=aarch64-linux-android24 --sysroot="$SYSROOT" \
    -resource-dir="$NDK/lib/clang/17" --ld-path="$NDK/bin/ld.lld" \
    -rtlib=compiler-rt --unwindlib=libunwind \
    -L"$SYSROOT/usr/lib/aarch64-linux-android/24" \
    -L"$NDK/lib/clang/17/lib/linux" \
    -O2 -Wall "$HERE/ksu_mode.c" -o "$OUT"
  echo "built $OUT (bionic dynamic)"
elif command -v aarch64-linux-gnu-gcc >/dev/null 2>&1; then
  aarch64-linux-gnu-gcc -static -O2 -Wall "$HERE/ksu_mode.c" -o "$OUT"
  aarch64-linux-gnu-strip "$OUT" 2>/dev/null || true
  echo "built $OUT (static glibc fallback)"
else
  echo "no suitable cross-compiler found" >&2; exit 1
fi
ls -la "$OUT"