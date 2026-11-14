# plk110 / 6.12.69 素材与定标记录

本目录存放适配目标二进制与定标记录。大体积 BTF dump 不入库（见根 .gitignore），需要时
按下述方法重新获取。

## 素材

`sched-walt.ko` 为 WALT 调速器本体。文件名用连字符，lsmod 显示的 `sched_walt` 是内核将
连字符转为下划线的结果。该模块为 [permanent]，从 vendor_boot ramdisk 早期加载，
/vendor_dlkm/lib/modules 中没有对应文件。

## 取得 sched-walt.ko

```
# 由激活 slot 决定 vendor_boot_a 或 _b
adb shell su -c 'getprop ro.boot.slot_suffix'        # 例如 _b
adb shell su -c 'dd if=/dev/block/sde67 of=/data/local/tmp/vb.img bs=4M'
# magiskboot 复制到 /data/local/tmp 后执行（/data/adb 下可能无法直接执行）
adb shell su -c 'cp /data/adb/ksu/bin/magiskboot /data/local/tmp/; cd /data/local/tmp && ./magiskboot unpack vb.img'
# vendor ramdisk 解出 ramdisk.cpio 后展开。设备上 SELinux 可能拒绝建目录，
# 可拉到 PC 用 tools/extract_cpio.py 解析 newc 格式提取 sched-walt.ko。
```

## 结构体偏移

由 walt_diag 读真机内存确定，已写入 device.h：

```
governor_data = *(policy + 0x58)        # wrapper
tunables      = *(governor_data + 0x28)
kobject       = tunables + 0x00         # name@0, ktype@40
hispeed_load  = tunables + 0xb0
zones         = tunables + 0xe0         # {u32 freq; u32 pct;}[10], freq==-1 终止
policy: cpu@0x1c, cpuinfo.max_freq@0x28, governor@0x50, governor_data@0x58
```

定标建议使用 common/walt_diag.c：cpufreq_cpu_get(0) 取得 policy，打印 governor_data 与
tunables，扫描 tunables+0..0x100 各偏移处的 name@+0 与 ktype@+40，两者均为内核指针
（ffffff...）的偏移即 kobject 所在。该模块只读，init 返回错误自动卸载。单靠反汇编推导
偏移容易出错，例如 sysfs show 包装里的 container_of 减 0x98 属于另一个 ktype。

## 关键函数

- `zone_max_util_pct_store`：band 解析器，接受空格分隔的 freq pct 对（最多 10 个 zone），
  写入后调用 update_util_inflate_factor。walt_tl 直接调用此函数。
- `update_util_inflate_factor(tunables, cluster)`：对每个 zone 计算
  `cluster+0x48+8i = cpu_scale*freq/max_freq`（频段边界）与
  `cluster+0x4c+8i = max_freq*100/pct`（util 膨胀系数）。pct 即目标负载。
- `waltgov_init`：kobject_init_and_add(x0=tunables)；governor_data 为 wrapper，
  [wrapper+0x28]=tunables。
- `waltgov_next_freq_shared`：调频决策（热路径，不修改）。

## kCFI 前导

```
python3 tools/read_preamble.py sched-walt.ko zone_max_util_pct_show zone_max_util_pct_store
  show  = 0x81d5d15c
  store = 0xa0cb31b9
```

WALT 自定义 sysfs_ops 以 (tunables, buf[, count]) 调用 attr->show/store，并校验函数前导
哈希。build.sh 将 tl_show/tl_store 的前导补丁为上述值。

## CRC

用 tools/extract_crcs.py 从导入相应符号的设备模块提取（__version_ext_crcs + __versions）：

```
python3 tools/extract_crcs.py <module.ko> | grep <symbol>
```

供体：sched-walt.ko（printk/string/cpufreq/kprobe）、zram_vendor.ko（sysfs_create_group）、
gh_ctrl.ko（sysfs_remove_group）、oplus_bsp_dfr_fdleak_check.ko（param_ops_string）。
module_layout=0x797f2b3e。param_ops_uint 与 param_ops_string 的 CRC 相同（按类型哈希）。

## 重新获取 BTF

```
adb shell su -c 'cat /sys/kernel/btf/vmlinux' > vmlinux_btf
# 用 pahole 或 BTF 解析脚本生成 btf_c.h，核对 struct cpufreq_policy 等布局
```