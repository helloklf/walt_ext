# OnePlus ACE7 Pro（PMS110 / OP64F1L1）/ 6.12.69-android16

- SoC：Snapdragon 8 Elite Gen 5（SM8850 系列，双簇 cpuinfo 3.63G / 4.61G），
  ColorOS 17（Android 17，PMS110_17.0.0.201(CN01)）
- 内核：`6.12.69-android16-6-g5fd39602326e-ab16372235-4k`
- WALT 模块：`sched-walt.ko`，位于 vendor_boot ramdisk，[permanent]，1014200 字节
- 两个 cpufreq policy（policy0 / policy6）

**适配状态：离机预适配，未真机验证。** 参数来自机主代采 dump
（tools/collect_ondevice.sh）+ PC 侧解包/反汇编/BTF 核对。首次真机接触时需完成
"验证"一节的全部步骤后方可下发。

## 与既有机型的关系

内核侧与 plk110（6.12.69-android16-6）同代际：BTF 六项结构布局逐值相同
（module 1600/init@392/exit@1528、kprobe 128、attribute_group attrs@0x18、
kernel_param 40、cpufreq_policy governor_data@0x58），16 个导入符号 CRC 与
plk110 全部一致（android16-6 KMI 冻结）。kCFI show/store 哈希相同。

vendor 侧是 oplus 系 WALT：zone 机制存在，原生无 target_loads（无节点冲突）。
zone_max_util_pct_store 无 write-once 守卫（写前清 -1 再重写），输入按升序、
内部不反转（与 plk110 相同，无需 TL_ZONES_DESCENDING / TL_ZONE_GUARD_SYM）。
zones 偏移 0xe0 —— 与 plk110（OnePlus 6.12）相同。三处偏移由 waltgov_init /
zone_show/store 反汇编确认，尚未 walt_diag 实测。

运行时指纹：真机 dump 读到 scmversion=g0e19e9baeede，与 ko modinfo 内嵌值一致
（注意与 ko vermagic 的 -g 后缀 g797637f5bc7e 不同，两者不可互推，
见 compare/README.md）。云端三元组匹配可用。

## 目录

```
device.h      vendor 偏移（zones@0xe0）、vermagic（ko 原值 -o-g797637f5bc7e）
crcs.h        导入符号 CRC 表（6.12 KMI，与 plk110 逐值相同）
config.sh     KVER=612、BRANCH_PROT=pac-ret（无 BTI）、kCFI 哈希
module.prop   KSU 元数据
materials/    sched-walt.ko、dump 素材与定标记录
out/          构建产物（gitignore）
module/       打包好的 KSU 模块
```

## 构建与安装

```
wsl -d Ubuntu-24.04 bash common/build.sh pms110/6.12.69
wsl -d Ubuntu-24.04 bash common/package.sh pms110/6.12.69
adb push pms110/6.12.69/module /data/local/tmp/walt_tl_module
adb shell su -c 'mkdir -p /data/adb/modules/walt_tl \
  && cp -r /data/local/tmp/walt_tl_module/* /data/adb/modules/walt_tl/ \
  && chmod 755 /data/adb/modules/walt_tl/*.sh'
```

## 验证（待真机完成，未完成前视为未适配）

1. `build_diag.sh` 构建 walt_diag，真机只读实测 governor_data→tunables(+0x28)、
   kobject(+0x00)、zones(+0xe0) 三处偏移，与 device.h 核对（反汇编初值）。
2. `insmod walt_tl.ko selftest=1`：检查格式转换往返（默认 SELFTEST_CFG
   `80 1200000:85 1900000:90` 的 OPP 有效性需顺带确认，不行则在 device.h
   覆盖 SELFTEST_CFG）。
3. 真实 sysfs 写入 + 回读；`cat /sys/module/walt_tl/version` 确认归属。
4. 重启/热重启后持久化验证。

## 注意事项

- 运行时写 target_loads 需要写 `sysfs_devices_system_cpu` 的 SELinux 权限。
- 定标过程与素材见 materials/README.md。
