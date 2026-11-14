# Xiaomi 18 Pro（M610BB / hongkong）/ 6.18.21-android17

- SoC：Snapdragon 8 Elite 6，HyperOS 4（Android 17）
- 内核：`6.18.21-android17-5-g284297031c53-abogki555938958-4k`
- WALT 模块：`sched-walt.ko`，位于 vendor_boot ramdisk，[permanent]，679936 字节
- 三个 cpufreq policy（cpu0 / cpu3 / cpu6）

## 与 6.12 机型的差异

WALT 的 band 机制本身一致（zone_max_util_pct、相关函数、kCFI 哈希都相同），common
逻辑直接复用。差异集中在内核版本与厂商结构：

内核 6.18（android17-5）相对 6.12（android16-6）的结构体变化，已在 common/walt_kern.h
中按 WALT_KVER 条件编译处理：

- struct module：1664 字节，init@376、exit@1536（6.12 为 1600 / 392 / 1528）。
- struct kprobe：120 字节（6.12 为 128），addr@40、symbol_name@48 不变。
- struct attribute_group：attrs@0x20（6.18 新增 bin_size@0x18；6.12 为 0x18）。
- kernel_param、cpufreq_policy（governor_data@0x58）等不变。

构建标志：6.18 启用 BTI，BRANCH_PROT=pac-ret+bti（6.12 为 pac-ret）。

KMI CRC 部分变化：module_layout=0x60801db3，cpufreq_cpu_get/put、register_kprobe、
sysfs_create_group/remove_group、param_ops_uint 均与 6.12 不同；printk/string/mem 类
CRC 不变。kCFI 哈希不变（show=0x81d5d15c，store=0xa0cb31b9）。

厂商 tunables 偏移：governor_data→tunables@+0x28、kobject@tunables+0 与 6.12 机型相同；
zones 偏移为 0xd8（OnePlus 0xe0、iQOO 0xf8、Xiaomi 0xd8，各家不同）。经 walt_diag 读内存
与 zone_show/store 反汇编（`[x20,#0xd8]`、`add x22,x0,#0xdc`）确认。

## 目录

```
device.h      vendor 偏移（zones@0xd8）、vermagic、符号名
crcs.h        导入符号 CRC 表（6.18 KMI）
config.sh     KVER=618、BRANCH_PROT=pac-ret+bti、kCFI 哈希
module.prop   KSU 元数据
materials/    sched-walt.ko 与定标记录
out/          构建产物（gitignore）
module/       打包好的 KSU 模块
```

## 构建与安装

```
wsl -d Ubuntu-24.04 bash common/build.sh hongkong/6.18.21
wsl -d Ubuntu-24.04 bash common/package.sh hongkong/6.18.21
adb push hongkong/6.18.21/module /data/local/tmp/walt_tl_module
adb shell su -c 'mkdir -p /data/adb/modules/walt_tl \
  && cp -r /data/local/tmp/walt_tl_module/* /data/adb/modules/walt_tl/ \
  && chmod 755 /data/adb/modules/walt_tl/*.sh && touch /data/adb/walt_tl.enable'
```

模块由 service.sh 在 late_start 阶段加载，为三个 policy 挂上 target_loads，默认保留
内核原有设定。

## 验证

- selftest：`80 1200000:85 1900000:90` 转换为 zones `1200000:80 1900000:85 3744000:90`，
  回读一致。
- 真实 sysfs 写入：写入成功，zone 转换正确。
- 无 oops。

## 注意事项

- 本机为临时 root，冷重启（adb reboot）会丢失 root，应使用热重启。加载脚本放在
  service.sh（late_start 阶段 root 已就绪），持久化触发按本机提权工具的实际流程。
- 运行时写 target_loads 需要写 `sysfs_devices_system_cpu` 的 SELinux 权限。
- 定标过程与素材见 materials/README.md。