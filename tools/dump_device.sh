#!/bin/bash
# Harvest per-device kernel/WALT material via adb into an output dir, for
# cross-device/cross-version comparison. usage: bash tools/dump_device.sh <outdir>
set -uo pipefail
export MSYS_NO_PATHCONV=1   # keep /data/... device paths intact under Git Bash
OUT="${1:?usage: dump_device.sh <outdir>}"
mkdir -p "$OUT"

adb get-state >/dev/null 2>&1 || { echo "no adb device"; exit 1; }

SH()  { adb shell "$@" 2>&1; }
SU()  { adb shell "su -c '$*'" 2>&1; }
LOG() { echo "$*"; }

LOG "== identity =="
SH "uname -a"                                        > "$OUT/uname.txt"
{ SH "getprop ro.product.device"; SH "getprop ro.product.model";
  SH "getprop ro.build.fingerprint"; SH "getprop ro.build.display.id";
  SH "getprop ro.system.build.version.release";
  SH "getprop ro.system.build.version.incremental";
  SH "getprop ro.vendor.build.security.patch";
  SH "getprop ro.boot.slot_suffix"; }                > "$OUT/props.txt"
SH "cat /proc/version"                               > "$OUT/proc_version.txt"
SH "cat /proc/cmdline"                               > "$OUT/cmdline.txt"
cat "$OUT/uname.txt" "$OUT/props.txt"

LOG "== root check =="
SU "id" | tee "$OUT/root_check.txt"

LOG "== walt module presence =="
SH "lsmod | grep -i walt"                            > "$OUT/lsmod_walt.txt"
SH "find /vendor /odm /system /vendor_dlkm /odm_dlkm -name '*walt*.ko' 2>/dev/null" > "$OUT/find_walt.txt"
SU "ls /sys/module/ | grep -i walt"                  > "$OUT/sys_module_walt.txt"
for f in version srcversion scmversion; do
  SU "cat /sys/module/sched_walt/$f 2>/dev/null"     > "$OUT/walt_module_$f.txt"
done
# scmversion (Xiaomi/Qualcomm vendor patch; often the only one populated) is the
# runtime fingerprint for sched-walt's source tree — see compare/README.md.
cat "$OUT/lsmod_walt.txt" "$OUT/find_walt.txt" "$OUT/sys_module_walt.txt"

LOG "== vendor_boot =="
SH "ls -l /dev/block/by-name/ | grep -i -E 'vendor_boot|boot'" > "$OUT/byname.txt"
SLOT=$(SH "getprop ro.boot.slot_suffix" | tr -d '\r')
SLOT="${SLOT:-_a}"
VB="vendor_boot${SLOT}"
LOG "dd $VB -> /data/local/tmp/vendor_boot.img"
SU "dd if=/dev/block/by-name/$VB of=/data/local/tmp/vendor_boot.img bs=4m" | tail -1
adb pull /data/local/tmp/vendor_boot.img "$OUT/vendor_boot.img" >/dev/null \
  && LOG "pulled vendor_boot.img ($(stat -c%s "$OUT/vendor_boot.img" 2>/dev/null) bytes)"

LOG "== BTF =="
SH "ls -l /sys/kernel/btf/"                          > "$OUT/btf_dir.txt"
SU "cat /sys/kernel/btf/vmlinux > /data/local/tmp/vmlinux.btf && chmod 644 /data/local/tmp/vmlinux.btf" > /dev/null
adb pull /data/local/tmp/vmlinux.btf "$OUT/vmlinux.btf" >/dev/null 2>&1 \
  && LOG "pulled vmlinux.btf ($(stat -c%s "$OUT/vmlinux.btf" 2>/dev/null) bytes)"

LOG "== cpufreq =="
{ for p in /sys/devices/system/cpu/cpufreq/policy*; do
    echo "--- $p"
    SU "cat $p/scaling_governor $p/cpuinfo_max_freq $p/scaling_max_freq 2>/dev/null"
    SU "cat $p/walt/zone_max_util_pct 2>/dev/null | head -c 200; echo"
    SU "ls $p/walt/ 2>/dev/null"
  done; }                                            > "$OUT/cpufreq.txt"
grep -c . "$OUT/cpufreq.txt" | xargs echo "cpufreq lines:"

LOG "== hashes =="
( cd "$OUT" && sha256sum vendor_boot.img vmlinux.btf 2>/dev/null ) | tee "$OUT/SHA256SUMS.txt"
echo "== dumped to $OUT =="
