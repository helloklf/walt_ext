# OnePlus 15（PLK110）/ 6.12.69-android16

- SoC：SM8850，ColorOS 17
- 内核：`6.12.69-android16-6-g242e1a07f878-ab15865446-4k`
- WALT 模块：`sched-walt.ko`，位于 vendor_boot ramdisk，[permanent]

## 目录

```
device.h      vendor 偏移、vermagic、目标符号名
crcs.h        导入符号 CRC 表
config.sh     MODNAME 与 kCFI 哈希（build.sh 读取）
module.prop   KSU 模块元数据
materials/    sched-walt.ko 与定标记录
out/          构建产物（gitignore）
module/       打包好的 KSU 模块（package.sh 生成）
```

## 构建与打包

```
wsl -d Ubuntu-24.04 bash common/build.sh plk110/6.12.69        # 编译，校验并补丁 kCFI
wsl -d Ubuntu-24.04 bash common/build_diag.sh plk110/6.12.69   # 偏移诊断模块
wsl -d Ubuntu-24.04 bash common/package.sh plk110/6.12.69      # 组装 KSU 模块到 module/
```

## 安装与使用

```
adb push plk110/6.12.69/module /data/local/tmp/walt_tl_module
adb shell su -c 'mkdir -p /data/adb/modules/walt_tl \
  && cp -r /data/local/tmp/walt_tl_module/* /data/adb/modules/walt_tl/ \
  && chmod 755 /data/adb/modules/walt_tl/*.sh && touch /data/adb/walt_tl.enable'
```

模块由 service.sh 在 late_start 阶段加载，为每个 WALT policy 挂上 target_loads，默认保留
内核原有设定。运行时接口：

```
cat /sys/devices/system/cpu/cpufreq/policy0/walt/target_loads
echo "80 1200000:85 1900000:90" > /sys/devices/system/cpu/cpufreq/policy0/walt/target_loads
insmod walt_tl.ko selftest=1     # 内核侧自检，开发验证用
```

## 验证

- selftest：`80 1200000:85 1900000:90` 转换为 zones `1200000:80 1900000:85 3628800:90`，
  回读一致。
- 真实 sysfs 写入：ColorOS 性能服务会主动写 target_loads，转换正确。
- 重启持久化：开机自动加载，无 oops。

## 注意事项

- 模块只提供 target_loads 接口，默认保留内核原有设定，不做开机配置。
- 运行时写 target_loads 需要写 `sysfs_devices_system_cpu` 的 SELinux 权限，ksu:s0 默认
  没有，厂商性能服务有。
- ColorOS 性能服务会在运行时动态覆盖 target_loads。
- 开发期间观察到 KSU root 间歇性掉权（id 偶尔为 2000），短 su 命令较稳定。
- 定标过程与素材见 materials/README.md。