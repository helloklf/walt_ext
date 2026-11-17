# pyb110 / 6.18.21 素材与定标记录

## 素材来源

全部素材来自机主代采的一份 dump（`tools/collect_ondevice.sh`，root 一键采集，
2026-11-17），未接触真机。本目录留存 ko 与小型文本记录；`vendor_boot.img`
（100MB，sha256 `e441efbb33f1165640f6dfe0cc64c653979c26cbf51214d6d3cef6707d8a1274`）
与 `vmlinux.btf`（7MB，sha256 `7a9b5be3b36883a2e458110485238887a0fb777415aa26aaa3a977f4ff27d509`）
按仓库惯例不入库，需要时用同法重新采集。

- `sched-walt.ko`：WALT 本体，1070576 字节，sha256
  `915da27c46fd2eff2574e2d4564d67890bb529943670bd0569938e69b4718e1d`。
  从 vendor_boot_a（/dev/block/sde34）解包取得：vendor_boot 含三个 ramdisk
  片段（platform / recovery / 16K），取 type=1 的 platform 片段（4k 页，
  LZ4 legacy）。解包链：tools/parse_vboot.py → tools/lz4_decode.py →
  tools/extract_cpio.py。
- `uname.txt` / `props.txt` / `cpufreq.txt` / `lsmod_walt.txt` / `byname.txt`：
  dump 原始记录。

## 6.18 结构体布局（BTF 核对）

tools/btf_offsets.py 读本机 BTF，与 hongkong/6.18.21 逐值相同：

```
struct module        size=1664  init@0x178(376)  exit@0x600(1536)
struct kprobe        size=120   addr@0x28  symbol_name@0x30
struct attribute_group  size=48  attrs@0x20
struct kernel_param  size=40
struct cpufreq_policy  size=744  governor_data@0x58  cpuinfo@0x28  governor@0x50
```

→ common/walt_kern.h 的 WALT_KVER=618 分支直接复用，无需新增。

## tunables 偏移（反汇编确认，walt_diag 待真机）

`waltgov_init`（sched-walt.ko）：

- `ldr/str [x19, #88]`：policy+0x58 = governor_data（wrapper，0x198 字节）。
- `str x21, [x20, #40]`：*(governor_data + 0x28) = tunables（0x128 字节）。
- `kobject_init_and_add(x0=x21, ...)`：kobject 嵌入 tunables+0x00。
- 初始化 stp -1 于 `[x21,#216]`..`[x21,#280]`：zones 数组在 tunables+0xd8，
  {u32 freq; u32 pct;}[10]。

`zone_max_util_pct_show`：`add x22, x0, #0xdc`（pct 字段），freq 在 x0+0xd8，
freq==-1 终止，循环 10 次。`zone_max_util_pct_store`：写前将 kobj+0xd8..+0x120
清为 -1 后按输入顺序重写（**无 write-once 守卫，重复写有效**；输入无需降序）。
.bss 的 `cluster_zones_initialized[4]`（0x14d4）只是 show 的"是否写过"标记
（store 写后置 1，show 读它决定回默认串还是已存 zones），不阻止重复写。

厂商默认值：policy0/3 内置 `2611200 90`（waltgov_init 从默认表拷贝 zones），
policy6 未初始化，show 回硬编码默认 `2147483647 80`。SELFTEST_RESTORE 取
`2611200 90`。

## kCFI 与 CRC

kCFI 前导（tools/read_preamble.py）：show=0x81d5d15c，store=0xa0cb31b9，
与 hongkong / 6.12 机型相同。

CRC（tools/extract_crcs.py；供体 sched-walt.ko、cpufreq_bouncing.ko、
qti_cpufreq_cdev.ko 及全 ramdisk 457 枚 ko 中筛出的 aiff_perf.ko〔sysfs_*〕、
inte.ko〔kprobe〕）：16 个导入符号与 hongkong/6.18.21 逐值相同
（module_layout=0x60801db3 等，android17-5 KMI 冻结）。

## vermagic 与构建标志

vermagic：`6.18.21-android17-5-o-maybe-dirty-4k SMP preempt mod_unload
modversions aarch64`（取自 sched-walt.ko 原值，`-o-` oplus 后缀 + dirty 树标记；
运行内核为 g7fcbffcad3e1，MODVERSIONS 下 same_magic() 只比标志位，见
compare/README.md）。

.note.gnu.property AARCH64 feature bits = PAC|BTI → BRANCH_PROT=pac-ret+bti。

## 运行时指纹：无

ko modinfo 无 srcversion；dump 中 /sys/module/sched_walt/{version,srcversion,
scmversion} 三节点均为空。本机云端匹配只能走 (brand, 内核版本) 唯一 ko 回退。
