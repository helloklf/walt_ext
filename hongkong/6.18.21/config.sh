# Build config for Xiaomi 18 Pro (M610BB / hongkong), 6.18.21-android17.
# Sourced by common/build.sh.
MODNAME=walt_tl
KVER=618                 # 6.18 GKI (android17)
BRANCH_PROT=pac-ret+bti  # 6.18 enables BTI (GNU property feature bits = PAC|BTI)

# kCFI type hashes (verified identical to the 6.12 devices — same WALT signatures).
KCFI_INIT=0x6fbb3035
KCFI_EXIT=0xe5c47d60
KCFI_SHOW=0x81d5d15c
KCFI_STORE=0xa0cb31b9