#!/bin/bash
# Build every device module and stage the artifacts into
#   kernel_modules/<brand>/<kernel-release>/<scmversion>.ko
# plus kernel_modules/MANIFEST.tsv (brand / release / scmversion / sha256 / path)
# for server-side deployment: match (brand, uname -r, /sys/module/sched_walt/scmversion)
# against the manifest; on miss, do NOT serve anything.
#
# usage: bash build-all.sh     (from Git Bash on Windows it re-execs under
#                               WSL Ubuntu-24.04 where the toolchain lives)
set -euo pipefail
export MSYS_NO_PATHCONV=1   # keep /mnt/... paths intact when calling wsl.exe from Git Bash
cd "$(dirname "$0")"

# The clang/lld toolchain lives in WSL; re-exec there when invoked elsewhere.
if ! grep -qi microsoft /proc/version 2>/dev/null; then
  wsl_path="$(pwd | sed -E 's|^/([a-zA-Z])(/[^ ]*)$|/mnt/\L\1\E\2|')/build-all.sh"
  echo "re-exec under WSL: $wsl_path"
  exec wsl -d Ubuntu-24.04 bash "$wsl_path" "$@"
fi

DEVICES=(hongkong/6.18.21 iqoo15/6.12.58 pjz110/6.6.118 plk110/6.12.69 pyb110/6.18.21)
OUT=kernel_modules
rm -rf "$OUT"
mkdir -p "$OUT"
printf 'brand\tkernel_release\tscmversion\tsha256\tpath\n' > "$OUT/MANIFEST.tsv"

for dev in "${DEVICES[@]}"; do
  echo "=== $dev ==="
  bash common/build.sh "$dev"
  bash common/package.sh "$dev" >/dev/null
  # shellcheck disable=SC1090
  source "$dev/config.sh"   # MODNAME, BRAND, SCHED_WALT_SCMVERSION
  # kernel release = first whitespace-separated token of PLK_VERMAGIC
  krel=$(sed -n '/PLK_VERMAGIC/,+2p' "$dev/device.h" \
         | grep -o '"[^"]*"' | head -1 | tr -d '"' | awk '{print $1}')
  scm="${SCHED_WALT_SCMVERSION%% *}"
  dest="$OUT/$BRAND/$krel/$scm.ko"
  mkdir -p "$(dirname "$dest")"
  cp "$dev/module/$MODNAME.ko" "$dest"
  sha=$(sha256sum "$dest" | awk '{print $1}')
  printf '%s\t%s\t%s\t%s\t%s\n' "$BRAND" "$krel" "$scm" "$sha" "$dest" >> "$OUT/MANIFEST.tsv"
  echo "-> $dest"
done

echo "== MANIFEST =="
column -t -s $'\t' "$OUT/MANIFEST.tsv"

# ---- universal KSU module: one flashable package for all devices ----------
# service.sh picks ko/<brand>/<kernel release>/<scmversion>.ko at boot.
DIST=dist/walt_tl
rm -rf dist
mkdir -p "$DIST/ko"
cp common/module_universal/module.prop common/module_universal/service.sh "$DIST/"
chmod 755 "$DIST/service.sh"
for d in "$OUT"/*/; do cp -r "$d" "$DIST/ko/"; done
echo "== universal module =="
find dist -type f | sort
python3 - <<'PY'
import os, zipfile
os.chdir("dist")
with zipfile.ZipFile("walt_tl-universal.zip", "w", zipfile.ZIP_DEFLATED) as z:
    for root, _, files in os.walk("walt_tl"):
        for f in files:
            p = os.path.join(root, f)
            z.write(p, p)
print("-> dist/walt_tl-universal.zip")
PY
ls -l dist/walt_tl-universal.zip
