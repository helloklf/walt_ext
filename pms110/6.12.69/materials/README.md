# pms110 / 6.12.69 素材与定标记录

## 素材来源

全部素材来自机主代采的一份 dump（`tools/collect_ondevice.sh`，root 一键采集，
2026-11-18），未接触真机。本目录留存 ko 与小型文本记录；`vendor_boot.img`
（100MB，未留哈希）与 `vmlinux.btf`（7MB，sha256
`19d0427e46d42f294418031f97cafac13bac3b73a7cb7b207874cd95b6cee033`）按仓库
惯例不入库，需要时用同法重新采集。

- `sched-walt.ko`：WALT 本体，1014200 字节，sha256
  `9d521f65bc609991d5be506f496106feed1ecaedf9699828c1d9ac3849a1dcc9`。
  从 vendor_boot_a 解包取得：vendor_boot v4 仅一个 ramdisk 片段（type=1
  platform，4k 页，LZ4 legacy）。解包链：tools/parse_vboot.py →
  tools/lz4_decode.py → tools/extract_cpio.py。
- `uname.txt` / `props.txt` / `cpufreq.txt` / `lsmod_walt.txt` / `byname.txt`：
  dump 原始记录。

## 6.12 结构体布局（BTF 核对）

tools/btf_offsets.py 读本机 BTF，与 plk110/6.12.69 逐值相同：

```
struct module        size=1600  init@0x188(392)  exit@0x5f8(1528)
struct kprobe        size=128   addr@0x28  symbol_name@0x30
struct attribute_group  size=40  attrs@0x18
struct kernel_param  size=40
struct cpufreq_policy  size=744  governor_data@0x58  cpuinfo@0x28  governor@0x50
```

→ common/walt_kern.h 的 WALT_KVER=612 分支直接复用，无需新增。

## tunables 偏移（反汇编确认，walt_diag 待真机）

`waltgov_init`（sched-walt.ko）：

- `ldr/str [x19, #0x58]`：policy+0x58 = governor_data（wrapper）。
- `str x21, [x20, #0x28]`：*(governor_data + 0x28) = tunables。
- show 直接以 x0（sysfs 传入的 kobject）+0xe0 读 zones → kobject 嵌入
  tunables+0x00。
- 初始化 stp -1 于 `[x21,#0xe0]`..`[x21,#0x120]`：zones 数组在 tunables+0xe0，
  {u32 freq; u32 pct;}[10]。

`zone_max_util_pct_show`：`ldr w3, [x24, #0xe0]`（freq），步进 8，freq==-1
终止。`zone_max_util_pct_store`：写前清 `[x20,#0xe0]`.. 为 -1 后按输入顺序
重写（**无 write-once 守卫，重复写有效**；输入无需降序）。

厂商默认值：policy0 内置 `2611200 90`（waltgov_init 从默认表拷贝 zones 到
tunables+0xe0），policy6 未初始化，show 回硬编码默认 `2147483647 80`。
SELFTEST_RESTORE 取 `2611200 90`。

## kCFI 与 CRC

kCFI 前导（tools/read_preamble.py）：show=0x81d5d15c，store=0xa0cb31b9，
与全部既有机型相同。

CRC：对 vendor_boot ramdisk 全部 505 枚 ko 用 tools/extract_crcs.py 扫描，
16 个导入符号与 plk110/6.12.69 逐值相同（module_layout=0x797f2b3e 等，
android16-6 KMI 冻结；供体例：coresight.ko〔sysfs_*〕、inte.ko〔kprobe〕、
camera_extension.ko〔param_ops_uint〕）。

## vermagic 与构建标志

vermagic：`6.12.69-android16-6-o-g797637f5bc7e-4k SMP preempt mod_unload
modversions aarch64`（取自 sched-walt.ko 原值；运行内核为 g5fd39602326e，
MODVERSIONS 下 same_magic() 只比标志位，见 compare/README.md）。

.note.gnu.property aarch64 feature = PAC（无 BTI）→ BRANCH_PROT=pac-ret。

## 运行时指纹

ko modinfo 内嵌 `scmversion=g0e19e9baeede`，与真机 dump 的
/sys/module/sched_walt/scmversion 读数一致；version/srcversion 节点为空。
云端三元组匹配可用（config.sh 的 SCHED_WALT_SCMVERSION）。
