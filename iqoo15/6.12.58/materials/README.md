# iQOO 15 / 6.12.58 素材与定标记录

## 素材

`sched-walt.ko` 为 WALT 本体，626688 字节，从 vendor_boot_a（/dev/block/sde24）解包取得。
本机 Magisk 未附带 magiskboot，故在 PC 侧解包：dd 导出镜像后，用 tools/parse_vboot.py
解析 vendor_boot v4 头与 ramdisk 表，提取 platform ramdisk（LZ4 legacy，魔数 0x184C2102），
用 tools/lz4_decode.py 解压，再用 tools/extract_cpio.py 提取 sched-walt.ko。

## 与 OnePlus 15 的一致性

两者同为 SM8850 的 Qualcomm WALT 实现：

- `zone_max_util_pct_store` 大小 0x454、`update_util_inflate_factor` 大小 0x1a4，与 OnePlus 一致。
- kCFI 前导（tools/read_preamble.py）：show=0x81d5d15c，store=0xa0cb31b9，与 OnePlus 一致。
- KMI CRC（tools/extract_crcs.py，供体 sched-walt.ko、common_hook.ko、adsp_loader_dlkm.ko）：
  module_layout=0x797f2b3e，_printk、memcpy、strlen、strsep、kstrtouint、scnprintf、
  cpufreq_cpu_get/put、register_kprobe/unregister_kprobe、sysfs_create_group/remove_group
  均与 OnePlus 相同（android16-6 GKI 稳定 KMI，6.12.58 与 6.12.69 一致）。

## 结构差异：zones 偏移

walt_diag 实测：kobject 在 tunables+0、hispeed_load 在 +0xb0，与 OnePlus 相同；但
zone (2611200,90) 位于 tunables+0xf8（OnePlus 为 +0xe0），终止符 -1 在 +0x100。

反汇编复核：zone_max_util_pct_show 读 `[x24,#0xf8]`、`[x24,#0xfc]`；
zone_max_util_pct_store 写 `[x20,#0xf8]`、`[x12,#0xf8]`、`[x11,#0xfc]`。

结论：vivo 的 tunables 结构在 zone 数组前多 0x18 字节，TL_ZONES_OFF 取 0xf8。

## vermagic

`6.12.58-android16-6-gfd60568f1f3a-4k SMP preempt mod_unload modversions aarch64`

取自 sched-walt.ko。运行内核为 gdd6fd5c98918，vivo 放宽 vermagic 校验至 KMI 世代。

## 复现工具

- `tools/parse_vboot.py <vb.img> [out.cpio]`：解析 vendor_boot v4，提取 platform ramdisk。
- `tools/lz4_decode.py <in.lz4> <out>`：LZ4 legacy 解压（纯 Python）。
- `tools/extract_cpio.py <cpio> [outdir]`：提取 walt/cpufreq 相关 .ko。
- `tools/read_preamble.py <ko> <func...>`：读 kCFI 前导。
- `tools/extract_crcs.py <ko>`：提取导入符号 CRC。
- `common/walt_diag.c`：只读偏移定标。