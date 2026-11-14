# iQOO 15（V2505A，vivo OriginOS 7，SM8850）/ 6.12.58-android16 适配清单

架构同一加：WALT zone_max_util_pct band 机制，common/walt_tl.c 翻译逻辑复用。
内核 6.12.58（一加是 6.12.69）→ CRC/结构体需重核。sched_walt 626688B [permanent]。

1. [x] 连接设备，确认架构（walt + zone_max_util_pct 同在）
2. [x] 取 sched-walt.ko（vendor_boot_a=sde24，PC 解包 parse_vboot+lz4_decode+extract_cpio）
3. [x] 核对 6.12.58 结构体布局（KMI 与 6.12.69 全同，module_layout=0x797f2b3e）
4. [x] walt_diag 实测定标偏移（gd+0x28 / kobj+0 / hispeed+0xb0 同；**zones=0xf8** 异于一加 0xe0）
5. [x] 收割 kCFI 前导（show=0x81d5d15c / store=0xa0cb31b9，与一加同）
6. [x] 收割 CRC（全同一加）+ vermagic（6.12.58-gfd60568f1f3a）
7. [x] 填 device.h / crcs.h / config.sh / module.prop（仅改 zones 偏移+vermagic+desc）
8. [x] 构建 + kCFI 校验（3/3 过）
9. [x] 真机 selftest（转换回读一致）+ 真实 sysfs 写入（vivo 服务写入正确转换）+ 手动加载 0 oops
   [x] 加载脚本改用 service.sh（late_start，永久/临时 root 均友好）；手动跑 service.sh 加载成功
   [ ] 持久化触发复验（本机临时 root，禁冷重启；简单 setprop 重启 zygote 不重跑 KSU
       service.sh，需按用户提权工具的热重启流程触发）
10. [x] 文档（机型 README + materials + 根 README 机型表 + tools）

## 已知
- governor: walt（available: walt conservative powersave performance schedutil，无 hmbird）
- zone_max_util_pct 默认 "2611200 90"，频率表同一加 policy0
- 关键函数同在：zone_max_util_pct_show/store, waltgov_next_freq_shared, waltgov_init
- kptr_restrict=2，slot _a