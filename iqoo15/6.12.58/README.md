# iQOO 15（V2505A / PD2505）/ 6.12.58-android16

- SoC：SM8850，vivo OriginOS 7
- 内核：`6.12.58-android16-6-gdd6fd5c98918-abogki543627742-4k`
- WALT 模块：`sched-walt.ko`，位于 vendor_boot ramdisk，[permanent]，626688 字节

## 与 OnePlus 15 的差异

两者同为 SM8850，WALT 实现基本一致：`zone_max_util_pct_store`、
`update_util_inflate_factor` 函数大小相同，kCFI 哈希相同（show=0x81d5d15c，
store=0xa0cb31b9），KMI CRC 相同（module_layout=0x797f2b3e，6.12.58 与 6.12.69 同属
android16-6 GKI 稳定 ABI），governor_data→tunables 偏移 +0x28、kobject 在 tunables+0、
hispeed_load 在 +0xb0 也一致。因此 crcs.h、config.sh 与 common 逻辑直接复用。

需要改动的只有两处：

- zones 偏移为 0xf8（OnePlus 为 0xe0）。vivo 的 tunables 结构在 zone 数组前多 0x18 字节。
  经 walt_diag 读内存与 zone_show/store 反汇编（`[x,#0xf8]`、`[x,#0xfc]`）确认。
- vermagic 为 6.12.58。

即 device.h 中的 TL_ZONES_OFF 与 PLK_VERMAGIC。

## 目录

```
device.h      vendor 偏移、vermagic、符号名
crcs.h        导入符号 CRC 表
config.sh     MODNAME 与 kCFI 哈希
module.prop   KSU 元数据
materials/    sched-walt.ko 与定标记录
out/          构建产物（gitignore）
module/       打包好的 KSU 模块
```

## 构建与安装

```
wsl -d Ubuntu-24.04 bash common/build.sh iqoo15/6.12.58
wsl -d Ubuntu-24.04 bash common/package.sh iqoo15/6.12.58
adb push iqoo15/6.12.58/module /data/local/tmp/walt_tl_module
adb shell su -c 'mkdir -p /data/adb/modules/walt_tl \
  && cp -r /data/local/tmp/walt_tl_module/* /data/adb/modules/walt_tl/ \
  && chmod 755 /data/adb/modules/walt_tl/*.sh'
```

模块由 service.sh 在 late_start 阶段加载，挂上 target_loads，默认保留内核原有设定。
target_loads 保持标准名称与位置（厂商性能服务会直接写它）；同目录出现 `walt_tl/` 目录
（内有只读 `version`，输出 `walt_tl <版本>`）即表示该节点由本模块提供。

## 验证

- selftest：`80 1200000:85 1900000:90` 转换为 zones `1200000:80 1900000:85 3628800:90`，
  回读一致。
- 真实 sysfs 写入：vivo 性能服务会主动写 target_loads（如 `90 787200:95`），转换正确。
- 手动执行 service.sh 加载正常，无 oops。

## 注意事项

- 本机为临时 root，冷重启（adb reboot）会丢失 root，应使用热重启。加载脚本放在
  service.sh（late_start 阶段 root 已就绪）。简单的 `setprop ctl.restart zygote` 不会
  重新执行 KSU service.sh，持久化触发需按本机提权工具的实际流程。
- 运行时写 target_loads 需要写 `sysfs_devices_system_cpu` 的 SELinux 权限，ksu:s0 默认
  没有，vivo 性能服务有。
- vivo 性能服务会在运行时动态覆盖 target_loads。
- 定标过程与素材见 materials/README.md。