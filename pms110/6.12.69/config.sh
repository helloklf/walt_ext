# Build config for OnePlus ACE7 Pro (PMS110 / OP64F1L1), 6.12.69-android16.
# Sourced by common/build.sh.
MODNAME=walt_tl
BRAND=oneplus            # ro.product.brand (lowercase) — kernel_modules/ artifact dir
KVER=612                 # 6.12 GKI (android16)
BRANCH_PROT=pac-ret      # .note.gnu.property AARCH64 feature bits = PAC only（无 BTI）

# kCFI type hashes (read_preamble.py on this device's sched-walt.ko;
# identical to plk110 and the other devices — same WALT signatures).
KCFI_INIT=0x6fbb3035
KCFI_EXIT=0xe5c47d60
KCFI_SHOW=0x81d5d15c
KCFI_STORE=0xa0cb31b9
# sched-walt 源码树指纹（真机 /sys/module/sched_walt/scmversion）。
# 云端下发按此匹配：真机指纹命中本列表才下发。
# g0e19e9baeede = 真机 dump 值，且与 ko modinfo 内嵌 scmversion 一致（本机 ko
# 带 scmversion 字段；注意与 ko vermagic 的 -g 后缀 g797637f5bc7e 不同，两者
# 不可互推，见 compare/README.md）。
SCHED_WALT_SCMVERSION="g0e19e9baeede"
