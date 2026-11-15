# Build config for OnePlus 15 (PLK110), 6.12.69-android16. Sourced by common/build.sh.
MODNAME=walt_tl
BRAND=oneplus           # ro.product.brand (lowercase) — kernel_modules/ artifact dir
KVER=612                 # 6.12 GKI (android16)
BRANCH_PROT=pac-ret      # 6.12 has no BTI

# kCFI type hashes the vendor WALT sysfs_ops checks against attr->show/store.
# These encode WALT's internal struct signature; a shadow-struct function pointer
# cannot reproduce them, so build.sh binary-patches tl_show/tl_store preambles to
# these values (the preamble is only a caller-side type tag).
#
# Harvest: read the 4 bytes immediately BEFORE the vendor function in its .text:
#   KCFI_SHOW  = preamble of zone_max_util_pct_show / hispeed_load_show
#   KCFI_STORE = preamble of zone_max_util_pct_store / hispeed_load_store
#   KCFI_INIT  = int(void) module-init hash (kernel-version-generic)
# e.g. llvm-objdump -s -j .text <ko> | look at func_addr-4, or tools/read_preamble.py
KCFI_INIT=0x6fbb3035
KCFI_EXIT=0xe5c47d60
KCFI_SHOW=0x81d5d15c
KCFI_STORE=0xa0cb31b9
# sched-walt 源码树指纹（真机 /sys/module/sched_walt/scmversion，空则回退 srcversion）。
# 云端下发按此匹配：真机指纹命中本列表才下发。
# g5e68b11425ae = 真机实测值（本机 scmversion 节点存在但为空，指纹实际来自 srcversion；
# walt_tl 验证适用）。真机解包的 ko 与 materials 逐字节相同，其 vermagic -g 后缀为
# gb79517a1cbef，与指纹不一致 —— scmversion/srcversion 均不能从 ko vermagic 推导
# （见 compare/README.md）。
SCHED_WALT_SCMVERSION="g5e68b11425ae"
