# pjz110 / 6.6.118 素材与定标记录

## 素材

`sched-walt.ko` 为 WALT 本体，536576 字节，从 vendor_boot_a（/dev/block/sde23）解包取得。
LZ4 legacy 压缩。解包链：tools/parse_vboot.py → tools/lz4_decode.py → tools/extract_cpio.py。
GKI BTF 取自 /sys/kernel/btf/vmlinux（binary-safe：su cp 到 /data/local/tmp 后 adb pull）。

## 6.6 结构体布局（tools/btf_offsets.py）

```
struct module        size=1536  init@0x188(392)  exit@0x5b8(1464)
struct kprobe        size=128   addr@0x28  symbol_name@0x30   （同 6.12）
struct attribute_group  attrs@0x18                            （同 6.12）
struct kernel_param  size=40                                  （同 6.12）
struct cpufreq_policy  governor_data@0x58  cpuinfo@0x28       （同 6.12）
```

仅 struct module 与 6.12 不同，在 common/walt_kern.h 按 WALT_KVER=606 分支处理。

## tunables 偏移（walt_diag 实测 + zone_show 反汇编）

```
governor_data = *(policy + 0x58)
tunables      = *(governor_data + 0x28)
kobject       = tunables + 0x00
zones         = tunables + 0xe0   （zone_show 读 [x24,#0xe0]/[x24,#0xe4]）
```

## zone_store 行为（反汇编 + 经验探测）

zone_max_util_pct_store（0x24438，0xa2c 字节）解析空格 freq pct 对，token 数须偶数，
pct 须 ≥80（cmn #0x15 校验），频率须降序（升序输入返回 -22）。用探测串确认：
- "1363200 80 1996800 85"（升序）→ -22
- "1996800 85 1363200 80"（降序）→ 接受

提交循环（0x24c5c）写 tunables+0xe0，但受 write_once_zone_max_util_pct_cluster
（.bss 守卫数组，4 cluster×4B）保护：cluster 已初始化（守卫非 0）则跳过写入。
update_util_inflate_factor（0x22c14）读 tunables+0xe0 计算 cluster 膨胀系数，故守卫
跳过时重复写无效。解决：tl_store 写前清零守卫数组 + memset zone 数组为 -1。

## kCFI 与 CRC

android15 kCFI 类型哈希与 android16 不同。前导（tools/read_preamble.py）：
init=0x36b1c5a6、exit=0xa540670c（cpufreq_bouncing.ko），show=0x7fdc93ec、
store=0x32a83d47（sched-walt.ko）。build.sh 补丁这四个前导。

CRC（tools/extract_crcs.py，供体 sched-walt.ko、inte.ko、adsp_loader_dlkm.ko、camera.ko）：
android15-8 KMI 与 android16-6 完全不同，module_layout=0x4e276f37，_printk=0x92997ed8 等
全部重新收割。

## vermagic 与构建标志

vermagic：`6.6.118-android15-8-o-g1524d9b076be-4k SMP preempt mod_unload modversions aarch64`
（取自 sched-walt.ko）。.note.gnu.property feature bits=0x2（BTI only），构建用 bti。

## 复现工具

tools/parse_vboot.py、lz4_decode.py、extract_cpio.py（解包）；btf_offsets.py（结构体）；
read_preamble.py、extract_crcs.py（kCFI/CRC）；common/walt_diag.c（偏移定标）。