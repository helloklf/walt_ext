# hongkong / 6.18.21 素材与定标记录

## 素材

`sched-walt.ko` 为 WALT 本体，679936 字节，从 vendor_boot_a（/dev/block/sde34）解包取得。
vendor_boot 含三个 ramdisk 片段（platform / recovery / 16K），取 type=1 的 platform 片段
（本机为 4k 页）。LZ4 legacy 压缩。解包链：tools/parse_vboot.py → tools/lz4_decode.py →
tools/extract_cpio.py。

GKI BTF 取自 /sys/kernel/btf/vmlinux。注意必须 binary-safe 获取（su cp 到 /data/local/tmp
后 adb pull）；直接 `adb shell cat` 会因终端字节处理损坏二进制。

## 6.18 结构体布局

用 tools/btf_offsets.py 解析 BTF 得到（与 6.12 的差异）：

```
struct module        size=1664  init@0x178(376)  exit@0x600(1536)   [6.12: 1600/392/1528]
struct kprobe        size=120   addr@0x28  symbol_name@0x30         [6.12: 128]
struct attribute_group  attrs@0x20  (bin_size@0x18)                 [6.12: attrs@0x18]
struct kernel_param  size=40   （同 6.12）
struct cpufreq_policy  governor_data@0x58  cpuinfo@0x28  governor@0x50  （同 6.12）
```

这些差异在 common/walt_kern.h 中按 WALT_KVER>=618 条件编译。

## tunables 偏移（walt_diag 实测 + 反汇编确认）

```
governor_data = *(policy + 0x58)
tunables      = *(governor_data + 0x28)
kobject       = tunables + 0x00   (name@0, ktype@40)
zones         = tunables + 0xd8   ({u32 freq; u32 pct;}[10], freq==-1 终止)
```

walt_diag 读内存：zone (2611200,90) 位于 *(tunables+0xd8)，终止符 -1 在 +0xe0。
反汇编复核：zone_max_util_pct_store 写 `[x20,#0xd8]`、`[x20,#0xe0]`；
zone_max_util_pct_show 用 `add x22,x0,#0xdc`（pct 字段）。

zones 偏移各家厂商不同：OnePlus 0xe0、iQOO 0xf8、Xiaomi 0xd8。

## kCFI 与 CRC

kCFI 前导（tools/read_preamble.py）：show=0x81d5d15c，store=0xa0cb31b9，与 6.12 机型相同。

CRC（tools/extract_crcs.py，供体 sched-walt.ko、hung_task_enhance.ko、
adsp_loader_dlkm.ko、dwc3-msm.ko）：module_layout=0x60801db3，cpufreq_cpu_get=0x2776b0a3，
cpufreq_cpu_put=0x1ea526c7，register_kprobe=0x100f01ea，unregister_kprobe=0x7ec06b6b，
sysfs_create_group=0xc332eb07，sysfs_remove_group=0x16ff3e85，param_ops_uint=0xae2dbea2。
printk/string/mem 类 CRC 与 6.12 相同。

## vermagic 与构建标志

vermagic：`6.18.21-android17-5-g32de97493654-mi-4k SMP preempt mod_unload modversions aarch64`
（取自 sched-walt.ko，带 -mi- 厂商后缀；运行内核为 g284297031c53，小米放宽 vermagic 至 KMI）。

.note.gnu.property 的 AARCH64 feature bits = 0x3（PAC|BTI），故构建用 pac-ret+bti。

## 复现工具

- tools/parse_vboot.py、lz4_decode.py、extract_cpio.py：vendor_boot 解包。
- tools/btf_offsets.py <btf> <struct...>：读结构体成员偏移。
- tools/read_preamble.py、extract_crcs.py：kCFI 与 CRC。
- common/walt_diag.c：只读偏移定标。