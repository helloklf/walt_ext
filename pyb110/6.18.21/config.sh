# Build config for OnePlus PYB110 (OP64E1L1), 6.18.21-android17.
# Sourced by common/build.sh.
MODNAME=walt_tl
BRAND=oneplus            # ro.product.brand (lowercase) — kernel_modules/ artifact dir
KVER=618                 # 6.18 GKI (android17)
BRANCH_PROT=pac-ret+bti  # .note.gnu.property AARCH64 feature bits = PAC|BTI

# kCFI type hashes (read_preamble.py on this device's sched-walt.ko;
# identical to hongkong/6.18.21 and the 6.12 devices — same WALT signatures).
KCFI_INIT=0x6fbb3035
KCFI_EXIT=0xe5c47d60
KCFI_SHOW=0x81d5d15c
KCFI_STORE=0xa0cb31b9
# 本机 sched-walt 无运行时指纹：ko 的 modinfo 里没有 srcversion，真机 dump 中
# /sys/module/sched_walt/{version,srcversion,scmversion} 三个节点均为空。
# 云端三元组匹配对本机不可用，加载靠 service.sh 的
# "该 (brand, 内核版本) 下唯一 ko" 回退 —— 因此占位名 "noscm" 只是文件名，
# 不参与指纹匹配；目录下必须保持唯一 .ko。
SCHED_WALT_SCMVERSION="noscm"
