# Xiaomi 18 Pro（M610BB / hongkong，HyperOS 4，骁龙 8E6）/ 6.18.21-android17 适配清单

WALT band 机制与 6.12 机型一致（zone_max_util_pct 同在，默认 2611200 90）。
关键差异：内核 6.18.21-android17-5（前两台的 android16-6 不同代）→ 结构体布局需重核，
构建标志带 BTI。临时 root，禁冷重启。

1. [x] 连接设备，确认架构（walt + zone_max_util_pct 同在，kptr_restrict=0）
2. [x] 取 sched-walt.ko（vendor_boot_a=sde34 解包）+ 6.18 GKI BTF（binary-safe pull）
3. [x] 核对 6.18 结构体：module 1664/init@376/exit@1536，kprobe 120，attribute_group
       attrs@0x20（多 bin_size）；kernel_param/cpufreq_policy 同 6.12
4. [x] 6.18 构建标志 pac-ret+bti；common/walt_kern.h 加 WALT_KVER 条件编译，
       build.sh 读 KVER/BRANCH_PROT；6.12 回归通过
5. [x] walt_diag 实测定标：gd+0x28 / kobj+0 同；**zones=0xd8**（反汇编复核）
6. [x] kCFI（0x81d5d15c/0xa0cb31b9 同）+ vermagic（6.18.21-...-mi-4k）+ 全部 CRC
       （module_layout=0x60801db3，cpufreq/kprobe/sysfs/param_ops 均变，printk/string 同）
7. [x] device.h / crcs.h / config.sh（KVER=618, pac-ret+bti）/ module.prop 完成
8. [x] 构建 + kCFI 校验（3/3 过，BTI）
9. [x] 真机 selftest（3 policy，往返一致）+ 真实 sysfs 写入（成功）+ 0 oops
   [ ] 持久化触发复验（临时 root，service.sh 已就位，按用户热重启流程触发）
10. [x] 文档（机型 README + materials + 根 README 机型表 + 适配指南）

## 已知
- governor: walt（available: walt scx conservative powersave performance schedutil）
- sched_walt 679936B [permanent]，zone_max_util_pct 默认 "2611200 90"
- 关键函数同在：zone_max_util_pct_show/store, update_util_inflate_factor, waltgov_next_freq_shared
- 频率表 8E6：288000..3744000，slot _a，kptr_restrict=0