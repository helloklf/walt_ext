#!/system/bin/sh
# On-device one-shot harvest of kernel/WALT adaptation material (run as root).
#
# usage:
#   adb push tools/collect_ondevice.sh /data/local/tmp/
#   adb shell "su -c 'sh /data/local/tmp/collect_ondevice.sh'"
#   (or run directly in any root shell on the device)
#
# Output: /sdcard/walt_dump_<device>_<ts>.tar[.gz]  (pull this file back to PC)
# Falls back to leaving the tarball in /data/local/tmp if /sdcard is not
# writable from the root mount namespace.
#
# Collected material mirrors tools/dump_device.sh (the adb-side harvester):
# identity/props, walt module presence + runtime fingerprints, any walt .ko
# found on the filesystem, vendor_boot.img, BTF, cpufreq/walt sysfs state.

set -u

[ "$(id -u 2>/dev/null)" = "0" ] || { echo "error: must run as root"; exit 1; }

TS=$(date +%Y%m%d_%H%M%S 2>/dev/null || echo unknown)
DEV=$(getprop ro.product.device 2>/dev/null | tr -d ' \r\n')
[ -n "$DEV" ] || DEV=unknown
NAME="walt_dump_${DEV}_${TS}"
BASE="/data/local/tmp/$NAME"
OUT="$BASE/$NAME"
mkdir -p "$OUT" || { echo "error: cannot create $OUT"; exit 1; }

LOG() { echo "$*"; }

LOG "== identity =="
{
  uname -a
  echo "--- /proc/version"
  cat /proc/version 2>/dev/null
} > "$OUT/uname.txt"
{
  for p in ro.product.device ro.product.model ro.product.brand \
           ro.build.fingerprint ro.build.display.id \
           ro.system.build.version.release ro.system.build.version.incremental \
           ro.vendor.build.security.patch ro.boot.slot_suffix; do
    echo "$p: $(getprop $p 2>/dev/null)"
  done
} > "$OUT/props.txt"
getprop > "$OUT/getprop_full.txt" 2>/dev/null
cat /proc/cmdline > "$OUT/cmdline.txt" 2>/dev/null
id > "$OUT/root_check.txt" 2>&1
cat "$OUT/uname.txt" "$OUT/props.txt"

LOG "== walt module presence =="
lsmod 2>/dev/null | grep -i walt > "$OUT/lsmod_walt.txt"
lsmod > "$OUT/lsmod_full.txt" 2>/dev/null
ls /sys/module/ 2>/dev/null | grep -i walt > "$OUT/sys_module_walt.txt"
for f in version srcversion scmversion; do
  node="/sys/module/sched_walt/$f"
  if [ -e "$node" ]; then
    val=$(cat "$node" 2>/dev/null)
    echo "${val:-<empty>}" > "$OUT/walt_module_$f.txt"
  else
    echo "<missing>" > "$OUT/walt_module_$f.txt"
  fi
done
# on-filesystem walt .ko (not present when it lives in vendor_boot ramdisk)
find /vendor /odm /system /vendor_dlkm /odm_dlkm -name '*walt*.ko' 2>/dev/null \
  > "$OUT/find_walt.txt"
if [ -s "$OUT/find_walt.txt" ]; then
  mkdir -p "$OUT/ko_fs"
  while IFS= read -r ko; do
    cp "$ko" "$OUT/ko_fs/" 2>/dev/null \
      && LOG "copied $ko"
  done < "$OUT/find_walt.txt"
fi
cat "$OUT/lsmod_walt.txt" "$OUT/find_walt.txt" "$OUT/sys_module_walt.txt"
echo "scmversion: $(cat "$OUT/walt_module_scmversion.txt" 2>/dev/null)"
echo "srcversion: $(cat "$OUT/walt_module_srcversion.txt" 2>/dev/null)"

LOG "== vendor_boot =="
ls -l /dev/block/by-name/ 2>/dev/null | grep -i -E 'vendor_boot|boot' \
  > "$OUT/byname.txt"
SLOT=$(getprop ro.boot.slot_suffix 2>/dev/null | tr -d '\r\n')
SLOT="${SLOT:-_a}"
VB="/dev/block/by-name/vendor_boot${SLOT}"
if [ -e "$VB" ]; then
  dd if="$VB" of="$OUT/vendor_boot.img" bs=4m 2>&1 | tail -1
else
  LOG "warn: $VB not found, trying without slot suffix"
  VB="/dev/block/by-name/vendor_boot"
  [ -e "$VB" ] && dd if="$VB" of="$OUT/vendor_boot.img" bs=4m 2>&1 | tail -1
fi

LOG "== BTF =="
ls -l /sys/kernel/btf/ > "$OUT/btf_dir.txt" 2>/dev/null
if [ -r /sys/kernel/btf/vmlinux ]; then
  cat /sys/kernel/btf/vmlinux > "$OUT/vmlinux.btf" 2>/dev/null \
    && LOG "vmlinux.btf: $(wc -c < "$OUT/vmlinux.btf") bytes"
fi

LOG "== cpufreq / walt sysfs =="
{
  for p in /sys/devices/system/cpu/cpufreq/policy*; do
    [ -d "$p" ] || continue
    echo "--- $p"
    echo "governor: $(cat "$p/scaling_governor" 2>/dev/null)"
    echo "cpuinfo_max_freq: $(cat "$p/cpuinfo_max_freq" 2>/dev/null)"
    echo "scaling_max_freq: $(cat "$p/scaling_max_freq" 2>/dev/null)"
    echo "available_frequencies: $(cat "$p/scaling_available_frequencies" 2>/dev/null)"
    if [ -d "$p/walt" ]; then
      echo "walt nodes: $(ls "$p/walt" 2>/dev/null | tr '\n' ' ')"
      echo "zone_max_util_pct: $(cat "$p/walt/zone_max_util_pct" 2>/dev/null)"
      echo "target_loads: $(cat "$p/walt/target_loads" 2>/dev/null)"
    else
      echo "walt nodes: <none>"
    fi
  done
} > "$OUT/cpufreq.txt"
cat "$OUT/cpufreq.txt"

LOG "== checksums =="
( cd "$OUT" && sha256sum vendor_boot.img vmlinux.btf ko_fs/*.ko 2>/dev/null ) \
  | tee "$OUT/SHA256SUMS.txt"

LOG "== packing =="
DEST="/sdcard"
[ -w "$DEST" ] || DEST="/data/local/tmp"
TARBASE="$DEST/$NAME.tar"
cd /data/local/tmp || exit 1
if tar -czf "$TARBASE.gz" "$NAME" 2>/dev/null; then
  PKG="$TARBASE.gz"
elif tar -cf "$TARBASE" "$NAME" 2>/dev/null; then
  PKG="$TARBASE"
else
  LOG "error: tar failed; raw material left in $BASE"
  exit 1
fi
chmod 644 "$PKG" 2>/dev/null

LOG "== done =="
LOG "package: $PKG ($(wc -c < "$PKG" 2>/dev/null) bytes)"
[ "$DEST" = "/sdcard" ] && LOG "pull with: adb pull $PKG"
