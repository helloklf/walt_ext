# Build config for OnePlus 13 (PJZ110), 6.6.118-android15. Sourced by common/build.sh.
MODNAME=walt_tl
KVER=606                 # 6.6 GKI (android15)
BRANCH_PROT=bti          # 6.6 android15: BTI only, no PAC (GNU property bits = 0x2)

# kCFI hashes for android15-8 (6.6). NOTE: the kCFI type-hash scheme differs from
# android16 (6.12+); these values are harvested from 6.6 device modules, not the
# 6.12 ones. init/exit from cpufreq_bouncing.ko, show/store from sched-walt.ko.
KCFI_INIT=0x36b1c5a6
KCFI_EXIT=0xa540670c
KCFI_SHOW=0x7fdc93ec
KCFI_STORE=0x32a83d47