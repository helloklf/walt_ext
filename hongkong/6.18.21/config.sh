# Build config for Xiaomi 18 Pro (M610BB / hongkong), 6.18.21-android17.
# Sourced by common/build.sh.
MODNAME=walt_tl
BRAND=xiaomi            # ro.product.brand (lowercase) — kernel_modules/ artifact dir
KVER=618                 # 6.18 GKI (android17)
BRANCH_PROT=pac-ret+bti  # 6.18 enables BTI (GNU property feature bits = PAC|BTI)

# kCFI type hashes (verified identical to the 6.12 devices — same WALT signatures).
KCFI_INIT=0x6fbb3035
KCFI_EXIT=0xe5c47d60
KCFI_SHOW=0x81d5d15c
KCFI_STORE=0xa0cb31b9
# sched-walt 源码树指纹（真机 /sys/module/sched_walt/scmversion）。
# 云端下发按此匹配：真机 scmversion 命中本列表才下发。
# gd9056cdb9ffb = 真机实测值，walt_tl 验证适用。
# 注意：本机 scmversion 与 materials/sched-walt.ko vermagic 的 -g 后缀（g32de97493654）
# 不一致，但两者 sha256 逐字节相同 —— scmversion 是独立记录的不透明指纹，不能从 ko
# vermagic 推导，建立映射只能靠真机验证（见 compare/README.md）。
SCHED_WALT_SCMVERSION="gd9056cdb9ffb"
