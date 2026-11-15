# OnePlus 13（PJZ110）/ 6.6.118-android15

- SoC：Snapdragon 8 Elite（SM8750），ColorOS 17
- 内核：`6.6.118-android15-8-g9bc34d5b0c79-abogki537459655-4k`
- WALT 模块：`sched-walt.ko`，位于 vendor_boot ramdisk，[permanent]，536576 字节
- 两个 cpufreq policy（cpu0 / cpu6）

## 与 6.12/6.18 机型的差异

WALT band 机制一致，但这台是 android15（6.6），差异最多：

内核结构体（common/walt_kern.h 按 WALT_KVER=606 处理）：
- struct module：1536 字节，init@392、exit@1464（6.12 为 1600/392/1528）。
- kprobe（128）、attribute_group（attrs@0x18）、kernel_param、cpufreq_policy 同 6.12。

kCFI 方案不同：android15 的类型哈希与 android16 不同，四个函数前导都要补丁成本机值
（init=0x36b1c5a6、exit=0xa540670c、show=0x7fdc93ec、store=0x32a83d47，从本机模块收割）。
build.sh 现在统一补丁 init/exit/show/store 四个前导。

构建标志：BTI only（.note.gnu.property feature bits=0x2，无 PAC），BRANCH_PROT=bti。

KMI CRC 全变：android15-8 与 android16-6 完全不同（连 printk/string/mem 都变），crcs.h
全部从本机模块重新收割，不能复用 6.12 值。

厂商 zone_store 三个特性（device.h 宏控制）：
- 输入频率须降序（TL_ZONES_DESCENDING）：升序输入被拒（-22）。zone_store 按输入降序
  原样存储，故 tl_show 也按降序反读。
- write-once 守卫（TL_ZONE_GUARD_SYM=write_once_zone_max_util_pct_cluster）：cluster
  初始化后 zone_store 跳过写入，重复写无效。tl_store 写前用 kallsyms_lookup_name 解析
  该 .bss 守卫数组并清零。
- 不清残留 zone：新配置 zone 数少于旧时不清理尾部，也不重写终止符。tl_store 写前
  memset zone 数组为 -1。

tunables 偏移：governor_data→tunables@+0x28、kobject@tunables+0、zones@+0xe0（同 OnePlus 15）。
selftest 频率须用本机有效 OPP（1363200/1996800；1200000/1900000 不是本机 OPP，会被拒）。

## 目录

```
device.h      vendor 偏移、vermagic、降序/守卫宏、selftest 频率
crcs.h        导入符号 CRC 表（6.6 KMI）
config.sh     KVER=606、BRANCH_PROT=bti、kCFI 哈希
module.prop   KSU 元数据
materials/    sched-walt.ko 与定标记录
out/          构建产物（gitignore）
module/       打包好的 KSU 模块
```

## 构建与安装

```
wsl -d Ubuntu-24.04 bash common/build.sh pjz110/6.6.118
wsl -d Ubuntu-24.04 bash common/package.sh pjz110/6.6.118
adb push pjz110/6.6.118/module /data/local/tmp/walt_tl_module
adb shell su -c 'mkdir -p /data/adb/modules/walt_tl \
  && cp -r /data/local/tmp/walt_tl_module/* /data/adb/modules/walt_tl/ \
  && chmod 755 /data/adb/modules/walt_tl/*.sh'
```

模块由 service.sh 在 late_start 阶段加载，挂上 target_loads，默认保留内核原有设定。
target_loads 保持标准名称与位置（厂商性能服务会直接写它）；同目录出现 `walt_tl/` 目录
（内有只读 `version`，输出 `walt_tl <版本>`）即表示该节点由本模块提供。

## 验证

- selftest：`80 1363200:85 1996800:90` 转换为降序 zones `3532800:90 1996800:85 1363200:80`
  （含 -1 终止符），回读一致。
- 真实 sysfs 写入：vivo 性能服务经 target_loads 写入 `85 1152000:90`，正确转换为降序
  zones `3532800 90 1152000 85`。
- 无 oops。

## 注意事项

- 本机为持久 root（重启后 root 仍在）。
- 运行时写 target_loads 需要写 `sysfs_devices_system_cpu` 的 SELinux 权限。
- vivo 性能服务会在运行时动态覆盖 target_loads。
- 定标过程与素材见 materials/README.md。