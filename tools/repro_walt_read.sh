#!/system/bin/sh
# bisect v2: which walt-dir read kills the kernel? markers go to STDOUT (host captures)
W0=/sys/devices/system/cpu/cpufreq/policy0/walt
W6=/sys/devices/system/cpu/cpufreq/policy6/walt

echo "PHASE-A-native-begin $(date +%T.%N)"
for p in $W0 $W6; do
  for n in zone_max_util_pct hispeed_load hispeed_freq hispeed_cond_freq pl adaptive_high_freq adaptive_low_freq boost rtg_boost_freq up_rate_limit_us down_rate_limit_us; do
    cat $p/$n >/dev/null 2>&1
  done
done
echo "PHASE-A-native-done $(date +%T.%N)"

echo "PHASE-B-version-begin $(date +%T.%N)"
cat $W0/walt_tl/version >/dev/null 2>&1
cat $W6/walt_tl/version >/dev/null 2>&1
echo "PHASE-B-version-done $(date +%T.%N)"

echo "PHASE-C-targetloads-begin $(date +%T.%N)"
cat $W0/target_loads >/dev/null 2>&1
cat $W6/target_loads >/dev/null 2>&1
echo "PHASE-C-targetloads-done $(date +%T.%N)"

echo "ALL-PHASES-DONE"
