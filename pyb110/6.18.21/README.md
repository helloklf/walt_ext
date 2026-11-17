# OnePlus 16（PYB110 / OP64E1L1）/ 6.18.21-android17

- SoC：Snapdragon 8 Elite Gen 5（SM8850 系列，cpuinfo 3.74G / 4.03G / 5.01G），
  ColorOS 17（Android 17，PYB110_17.0.1.201(CN01)）
- 内核：`6.18.21-android17-5-g7fcbffcad3e1-ab16353812-4k`
- WALT 模块：`sched-walt.ko`，位于 vendor_boot ramdisk，[permanent]，1070576 字节
- 三个 cpufreq policy（policy0 / policy3 / policy6）

**适配状态：已真机验证。** 参数来自机主代采 dump（tools/collect_ondevice.sh）
+ PC 侧解包/反汇编/BTF 核对的离机预适配，真机测试通过（2026-11）。

## 与既有机型的关系

内核侧与 hongkong（6.18.21-android17-5）同代际：BTF 六项结构布局逐值相同
（module 1664/init@376/exit@1536、kprobe 120、attribute_group attrs@0x20、
kernel_param 40、cpufreq_policy governor_data@0x58），16 个导入符号 CRC 与
hongkong 全部一致（KMI 冻结再次成立）。kCFI show/store 哈希相同。

vendor 侧是 oplus 系 WALT：zone 机制存在，原生无 target_loads（无节点冲突）。
zone_max_util_pct_store 无 write-once 守卫（写前清 -1 再重写，重复写有效），
输入按升序、内部不反转（与 plk110/hongkong 相同，无需 TL_ZONES_DESCENDING /
TL_ZONE_GUARD_SYM）。zones 偏移 0xd8 —— 与 OnePlus 6.12（0xe0）不同，与
Xiaomi 6.18（0xd8）相同。偏移由 waltgov_init / zone_show/store 反汇编确认，
尚未 walt_diag 实测。

本机 sched-walt **无运行时指纹**：ko modinfo 无 srcversion，dump 中
/sys/module/sched_walt/{version,srcversion,scmversion} 三个节点均为空。云端
三元组匹配不可用，加载依赖 service.sh 的"该 (brand, 内核版本) 下唯一 ko"
回退（config.sh 里 SCHED_WALT_SCMVERSION="noscm" 为占位文件名）。

## 目录

```
device.h      vendor 偏移（zones@0xd8）、vermagic（ko 原值 -o-maybe-dirty）
crcs.h        导入符号 CRC 表（6.18 KMI，与 hongkong 逐值相同）
config.sh     KVER=618、BRANCH_PROT=pac-ret+bti、kCFI 哈希
module.prop   KSU 元数据
materials/    sched-walt.ko、dump 素材与定标记录
out/          构建产物（gitignore）
module/       打包好的 KSU 模块
```

## 构建与安装

```
wsl -d Ubuntu-24.04 bash common/build.sh pyb110/6.18.21
wsl -d Ubuntu-24.04 bash common/package.sh pyb110/6.18.21
adb push pyb110/6.18.21/module /data/local/tmp/walt_tl_module
adb shell su -c 'mkdir -p /data/adb/modules/walt_tl \
  && cp -r /data/local/tmp/walt_tl_module/* /data/adb/modules/walt_tl/ \
  && chmod 755 /data/adb/modules/walt_tl/*.sh'
```

## 验证（2026-11 真机完成）

1. 三处偏移（governor_data→tunables +0x28、kobject +0x00、zones +0xd8）来自
   反汇编，真机验证通过（未另行 walt_diag 定标）。
2. `insmod walt_tl.ko selftest=1` 格式转换往返正常；真实 sysfs 写入、回读正常。

## 注意事项

- 运行时写 target_loads 需要写 `sysfs_devices_system_cpu` 的 SELinux 权限。
- 定标过程与素材见 materials/README.md。
