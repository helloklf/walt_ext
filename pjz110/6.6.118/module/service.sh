#!/system/bin/sh
# walt_tl injector — runs in the service (late_start) stage rather than
# post-fs-data. By late_start the root daemon is up for BOTH permanent and
# temporary root, so this is friendly to both (temporary root misses the early
# post-fs-data window). Adds the target_loads capability; kernel tuning untouched.
# Enabled by default; uninstall the module in KSU manager to disable.
MODDIR=${0%/*}
LOG=$MODDIR/boot.log

echo "start $(date) uptime=$(cat /proc/uptime)" > $LOG

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
    insmod $MODDIR/walt_tl.ko
    rc=$?
    echo "insmod try$i rc=$rc uptime=$(cat /proc/uptime)" >> $LOG
    [ $rc -eq 0 ] && break
    sleep 1
  done
) &