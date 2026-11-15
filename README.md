# walt_ext

为高通 WALT cpufreq 调速器提供分频段目标负载（target_loads）支持，按机型 / 内核版本组织。

WALT 没有 interactive 那样的 target_loads，但自带 `zone_max_util_pct` 机制：以空格分隔的
「频率 百分比」对定义各频段的目标负载，`update_util_inflate_factor()` 据此计算 util 膨胀
系数参与调频。本项目在此基础上增加一个 target_loads 接口，接受 interactive 格式：

```
80 1200000:85 1900000:90
```

即默认目标负载 80%，1200000kHz 以上 85%，1900000kHz 以上 90%。模块把该格式转换为 WALT
的 zone 表示，调用厂商原有的 `zone_max_util_pct_store()` 完成写入，解析、校验、加锁和
膨胀系数计算均沿用厂商实现，不修改调频路径（`waltgov_next_freq_shared`）。

转换关系：

```
80 1200000:85 1900000:90  ->  (1200000,80) (1900000,85) (max_freq,90)
```

## 目录结构

```
common/                  跨机型共享代码
  walt_kern.h            GKI 内核定义（结构体、宏、导入声明），按 WALT_KVER 区分 6.12/6.18
  walt_tl.c              解析、格式转换、sysfs、selftest、模块生命周期
  walt_diag.c            只读偏移诊断，适配新机型时用
  build.sh / build_diag.sh / package.sh / module.lds
  module/                KSU 脚本模板（service.sh）
tools/                   素材收割工具
  dump_device.sh         连接测试机一键采集内核/WALT 素材到 compare/<机型>-<版本>/
  extract_crcs.py        从 .ko 提取导入符号 CRC
  read_preamble.py       读函数 kCFI 前导哈希
  btf_offsets.py         从 BTF 读结构体成员偏移
  parse_vboot.py         解析 vendor_boot v4 头，提取 platform ramdisk
  lz4_decode.py          LZ4 legacy 解压（纯 Python）
  extract_cpio.py        解析 newc cpio，提取 .ko
<机型>/<内核版本>/        机型相关配置与素材
  device.h               vendor 偏移、vermagic、目标符号名
  crcs.h                 导入符号 CRC 表
  config.sh              MODNAME 与 kCFI 哈希
  module.prop            KSU 元数据
  materials/             目标 .ko 与定标记录
  README.md              机型文档
compare/                 跨机型/跨版本对比实验素材与报告（见 compare/README.md）
kernel_modules/          服务器下发产物树（build-all.sh 生成，gitignore）：brand/内核版本/scmversion.ko
build-all.sh             批量构建全部机型并归集产物 + MANIFEST.tsv
```

通用逻辑集中在 common/。适配新机型一般只需新增一个 `<机型>/<内核>/` 目录，填写
device.h、crcs.h、config.sh 三个配置；内核版本相同时 common/walt_kern.h 可复用。

## 构建与安装

```
wsl -d Ubuntu-24.04 bash common/build.sh plk110/6.12.69
wsl -d Ubuntu-24.04 bash common/package.sh plk110/6.12.69
```

产物在 `plk110/6.12.69/module/`，安装到 `/data/adb/modules/walt_tl`，步骤见各机型 README。
模块通过 KSU 的 service.sh（late_start 阶段）加载，该阶段 root 已就绪，对永久 root 和
临时 root 都适用。

批量构建与下发产物：`bash build-all.sh`（Git Bash 下自动转入 WSL 执行）会把全部机型
构建并归集到 `kernel_modules/<brand>/<内核版本>/<sched-walt scmversion>.ko`，同时生成
`kernel_modules/MANIFEST.tsv`（brand / kernel_release / scmversion / sha256 / path）。
服务器按客户端上报的 (brand, uname -r, /sys/module/sched_walt/scmversion) 三元组查
manifest，命中才下发，匹配不到不下发。

同一脚本还会组装通用 KSU 模块 `dist/walt_tl/` 并打包为 `dist/walt_tl-universal.zip`：
模块内含全部机型的 ko 树，service.sh 开机时自动按 (brand, uname -r, scmversion)
三元组选择匹配的 ko 加载；scmversion 读取不到时退化为"该 (brand, 内核版本) 下唯一
ko"；无匹配则安静跳过（不加载 = 原生行为）。模板在 `common/module_universal/`。

## 适配新机型

1. 取得 WALT 模块。`lsmod | grep walt` 确认模块名（lsmod 显示下划线，文件名可能是连字符）。
   若模块在 vendor_boot ramdisk 中（[permanent]，文件系统里找不到），解包 vendor_boot，
   见 tools/parse_vboot.py、lz4_decode.py、extract_cpio.py 及各机型 materials/README.md。
2. 确认 band 机制。反汇编定位 `zone_max_util_pct_store` 与 `update_util_inflate_factor`，
   确认百分比到膨胀系数的语义。注意厂商差异：zone 输入/存储可能要求降序
   （TL_ZONES_DESCENDING）；zone_store 可能有 write-once 守卫（TL_ZONE_GUARD_SYM，写前清零）
   且不清残留 zone（写前 memset）。用探测串确认接受格式。若机制不同，调整 common/walt_tl.c。
3. 定标偏移。用 build_diag.sh 构建 walt_diag，读真机内存确定 governor_data→tunables、
   kobject、zones 三处偏移，填入 device.h。建议以实测为准：sysfs show 包装里的
   container_of 偏移可能属于另一个 ktype，单看反汇编容易判断错。
4. 收割 kCFI 哈希。`read_preamble.py <ko> zone_max_util_pct_show zone_max_util_pct_store`，
   填入 config.sh。
5. 收割 CRC。用 extract_crcs.py 从导入相应符号的设备模块提取，填入 crcs.h；同时核对
   vermagic 与 module_layout CRC。
6. 核对内核定义。用 btf_offsets.py 读 BTF，核对 struct module、kprobe、kernel_param、
   cpufreq_policy、attribute_group 的布局。common/walt_kern.h 按 WALT_KVER 条件编译区分
   内核版本（现有 612/618）；新版本若有结构差异，在其中补充对应分支。config.sh 里设置
   KVER 与 BRANCH_PROT（6.18 起启用 BTI，用 pac-ret+bti；是否带 BTI 看 .note.gnu.property
   的 AARCH64 feature bits）。
7. 构建并验证。build.sh 会校验 kCFI；`insmod walt_tl.ko selftest=1` 检查格式转换往返，
   再做真实 sysfs 写入与加载持久化验证。临时 root 机型用热重启，不要冷重启。

## 安全性

- 不修改 WALT 本体，卸载即恢复；调频路径不变。
- 仅新增冷路径 sysfs，模块退出时移除属性组。切换 governor 会销毁 tunables kobject，
  target_loads 文件随之移除，不会悬空。
- target_loads 保持标准名称与位置（厂商性能服务会直接写它）。为便于区分节点归属，
  模块在每个 policy 的 walt 目录下额外创建 `walt_tl/version` 只读标记；`ls` 到 `walt_tl/`
  目录即表明旁边的 target_loads 由本模块提供。
- 模块只提供 target_loads 接口，默认保留内核原有设定，不做开机配置。运行时写入 sysfs
  需要相应的 SELinux 权限（ksu:s0 默认没有，厂商性能服务有）。
- 偏移或结构体布局错误可能导致崩溃，适配时先用 walt_diag 只读定标，再启用 sysfs。

## 已适配机型

| 机型 | 系统 | 内核 | 状态 |
|---|---|---|---|
| OnePlus 15（PLK110，SM8850） | ColorOS 17 | 6.12.69-android16 | 已验证：selftest、真实写入、重启持久化 |
| iQOO 15（V2505A，SM8850） | OriginOS 7 | 6.12.58-android16 | 已验证：selftest、真实写入；zones 偏移 0xf8；持久化按临时 root 流程触发 |
| Xiaomi 18 Pro（M610BB，8 Elite 6） | HyperOS 4 | 6.18.21-android17 | 已验证：selftest、真实写入；6.18 结构体/BTI 适配，zones 偏移 0xd8；持久化按临时 root 流程触发 |
| OnePlus 13（PJZ110，8 Elite） | ColorOS 17 | 6.6.118-android15 | 已验证：selftest、真实写入；6.6 结构体/kCFI 方案/BTI，降序 zone + write-once 守卫处理 |

## 协议

Copyright (C) 2026 嘟嘟ski

本项目采用 GNU General Public License v3.0 or later，全文见 LICENSE。衍生项目须以相同
协议开源其对应源码；GPL 允许商业使用，不附加非商业限制；软件不提供任何担保。

内核模块 modinfo 声明 license=GPL，这是调用 GPL-only 内核符号（sysfs_create_group、
register_kprobe 等）的要求，与 GPL-3.0 一致。