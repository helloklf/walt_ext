#!/bin/bash
# Assemble the installable KSU module into <devdir>/module/ from the generic
# scripts (common/module/), the device module.prop, and the built .ko.
# usage: wsl -d Ubuntu-24.04 bash common/package.sh <device/kernel dir>
set -euo pipefail
COMMON="$(cd "$(dirname "$0")" && pwd)"
if [ $# -lt 1 ]; then echo "usage: package.sh <device/kernel dir>"; exit 1; fi
DEVDIR="$(cd "$1" && pwd)"
# shellcheck disable=SC1091
source "$DEVDIR/config.sh"   # MODNAME
M="$DEVDIR/module"
mkdir -p "$M"
cp "$COMMON/module/service.sh" "$M/"
cp "$DEVDIR/module.prop" "$M/"
cp "$DEVDIR/out/$MODNAME.ko" "$M/walt_tl.ko"
chmod 755 "$M"/*.sh
echo "packaged KSU module -> $M"
ls -la "$M"