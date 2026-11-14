# OnePlus 13（PJZ110，ColorOS 17，骁龙 8 Elite SM8750）/ 6.6.118-android15 适配清单

WALT band 机制一致（zone_max_util_pct 同在，默认 4 zone 全 80）。
内核 6.6.118-android15-8（第三套内核版本）→ 结构体需重核，WALT_KVER=606。

1. [x] 连接设备，确认架构（walt + zone_max_util_pct 同在）
2. [x] 取 sched-walt.ko（vendor_boot_a=sde23）+ 6.6 GKI BTF（binary-safe）
3. [x] 核对 6.6 结构体：module 1536/init@392/exit@1464；其余同 6.12
4. [x] common/walt_kern.h 加 WALT_KVER=606；BTI only（feature bits=0x2）
5. [x] walt_diag 定标：gd+0x28 / kobj+0 / zones+0xe0（同一加 15）
6. [x] kCFI（android15 方案，四前导全补丁）+ CRC（KMI 全变）+ vermagic
7. [x] device.h / crcs.h / config.sh / module.prop
8. [x] 构建 + kCFI 校验（4/4 过）
9. [x] selftest 往返一致 + 真实 sysfs 写入（vivo 服务写入正确转换）+ 0 oops；持久 root
   额外处理：zone 降序输入/存储（TL_ZONES_DESCENDING）、write-once 守卫清零、写前清残留 zone
10. [x] 文档（机型 README + materials + 根 README 机型表）

## 已知
- governor walt，sched_walt 536576B [permanent]
- zone_max_util_pct 默认 "1324800 80 1747200 80 2112000 80 2438400 80"
- 关键函数同在，kptr_restrict=2，slot _a