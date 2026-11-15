#!/system/bin/sh
# walt_tl universal injector — auto-selects the .ko matching THIS device:
#   <MODDIR>/ko/<brand>/<kernel release>/<sched-walt scmversion>.ko
# where brand   = getprop ro.product.brand (lowercased),
#       release = uname -r,
#       scm     = /sys/module/sched_walt/scmversion (fallback: srcversion).
# Exact triple match first; if scm is unreadable/missing on this build, fall back
# to the unique .ko under (brand, release). No match -> skip entirely
# (module absent = stock behavior, safe). Adds target_loads only; no kernel
# tuning. Enabled by default; uninstall the module in KSU manager to disable.
MODDIR=${0%/*}
LOG=$MODDIR/boot.log

echo "start $(date) release=$(uname -r)" > $LOG
BRAND=$(getprop ro.product.brand | tr 'A-Z' 'a-z')
if [ -z "$BRAND" ]; then echo "no brand, skip" >> $LOG; exit 0; fi
KREL=$(uname -r)
SCM=$(cat /sys/module/sched_walt/scmversion 2>/dev/null)
if [ -z "$SCM" ]; then SCM=$(cat /sys/module/sched_walt/srcversion 2>/dev/null); fi
echo "probe brand=$BRAND krel=$KREL scm=$SCM" >> $LOG

KO=""
if [ -n "$SCM" ] && [ -f "$MODDIR/ko/$BRAND/$KREL/$SCM.ko" ]; then
  KO="$MODDIR/ko/$BRAND/$KREL/$SCM.ko"
else
  # fallback: exactly one candidate under this (brand, release)
  DIR="$MODDIR/ko/$BRAND/$KREL"
  N=$(ls "$DIR" 2>/dev/null | grep -c '\.ko$')
  if [ "$N" = "1" ]; then
    KO="$DIR/$(ls "$DIR" | grep '\.ko$')"
    echo "scm miss, unique krel fallback" >> $LOG
  fi
fi
if [ -z "$KO" ]; then echo "no match, skip" >> $LOG; exit 0; fi
echo "selected ${KO#$MODDIR/}" >> $LOG

# Background so late_start is not blocked. Wait for WALT governor active (usually
# already up by this stage), then insmod with retries.
(
  for i in $(seq 1 60); do
    if grep -q walt /sys/devices/system/cpu/cpufreq/policy0/scaling_governor 2>/dev/null \
       && [ -e /sys/devices/system/cpu/cpufreq/policy0/walt/zone_max_util_pct ]; then
      break
    fi
    sleep 0.5
  done
  echo "walt ready uptime=$(cat /proc/uptime)" >> $LOG
  for i in $(seq 1 10); do
    insmod "$KO"
    rc=$?
    echo "insmod try$i rc=$rc uptime=$(cat /proc/uptime)" >> $LOG
    [ $rc -eq 0 ] && break
    sleep 1
  done
) &
