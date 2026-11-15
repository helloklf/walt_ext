# 对比实验：HyperOS 2 (Mix Flip 2) vs HyperOS 4 (小米 15)

目的：实测系统版本差距很大的两台机器（同 SoC 代、同 GKI 代际 6.6-android15）之间，
内核侧参数（结构布局 / CRC / kCFI）与 vendor 侧参数（sched-walt 机制）各自变化有多大，
为云端自动下发与 OTA 后快速适配提供依据。

采集：`bash tools/dump_device.sh compare/<机型>-<系统版本>`（身份 / vendor_boot / BTF /
cpufreq / sched_walt 指纹），PC 侧解包 + `extract_crcs.py` / `read_preamble.py` /
`btf_offsets.py` 收割参数。目录命名约定：`compare/<ro.product.device>-<ro.build.display.id 主版本>/`，
同一台机器系统升级后新开目录，纵向 diff 跟踪（见文末）。

## 样本

| | bixi-2.0.213（Mix Flip 2） | dada-4.0.19（小米 15） |
|---|---|---|
| 系统 | HyperOS 2.0.213 / Android 15 | HyperOS 4.0.19 / Android 17 |
| uname -r | 6.6.56-android15-8-g7eb9096ae569-4k | 6.6.118-android15-8-gb9cc6ec16bc8-4k |
| sched-walt.ko | 1114056 字节，scmversion g7eb9096ae569 | 1188056 字节，scmversion gbc807ab2c5a8 |
| ko vermagic | 6.6.56-android15-8-... | **6.6.57**-android15-8-...（≠ 运行内核！） |
| walt sysfs | target_loads 等，**无 zone_max_util_pct** | **两者都有**：target_loads + zone_max_util_pct |

## 对比结果

| 维度 | 结果 |
|---|---|
| BTF 结构布局（module/kprobe/kernel_param/attribute_group/cpufreq_policy） | **完全一致**（module=1536, kprobe=128, param=40, ag=40, policy=760） |
| module_layout CRC | **相同** 0x4e276f37 |
| 导入符号 CRC 表 | 各 339 项，交集 297 项，**值零变化**；差异仅为各自新增的符号（bixi 多 metis_*，dada 多 plat_game_*/vip_* 及一批 tracepoint） |
| kCFI 前导（show/store 签名哈希） | **相同**：0x7fdc93ec / 0x32a83d47（与 pjz110 config.sh 的 KCFI_SHOW/STORE 也相同） |
| vendor 调频机制 | **大变**：HyperOS2 无 zone 机制、自带 interactive 风格 target_loads（值为负数编码）；HyperOS4 新增 zone_max_util_pct，保留原生 target_loads |
| vermagic 现象 | dada 的 vendor 模块 vermagic 是 6.6.57，在 6.6.118 内核上正常加载 |

## 结论

1. **KMI 冻结实证**：6.6.56 → 6.6.118 跨系统大版本，内核侧 ABI（布局 + 全部 CRC）零变化。
   内核侧参数只需按 GKI 代际（6.6-android15 / 6.12-android16 / 6.18-android17）归档，
   代际内任何小版本直接复用。
2. **vermagic 修正**：内核启用 MODVERSIONS 时，`same_magic()` 跳过版本串、只比对空格后
   的标志位（SMP/preempt/...），版本兼容性由逐符号 CRC 实际把关。所谓"只校验前几位"的
   真实机制即此——OTA 小版本变化后 old vermagic 的 ko 只要 CRC 表不变就**仍能加载**。
   这意味着之前"OTA 必改 PLK_VERMAGIC"的说法偏保守：严格做法是核对 CRC 表，vermagic
   按新 uname -r 重刷更干净但不是硬性前提。
3. **vendor 侧不可假设**：同厂商跨系统版本，sched-walt 机制可以大改（增删整套 tunable）。
   云端匹配必须带 fingerprint/内核完整版本，且逐机型核对机制（符号名集合），不能只看
   内核前三位。
4. **节点重名冲突**：HyperOS4 的原生 `target_loads` 已存在于我们欲创建的路径，
   `sysfs_create_group` 会 -EEXIST 导致加载失败。Xiaomi 机型适配需特判：原生已有
   target_loads 的机型直接跳过（无适配价值），或换节点名。
5. **HyperOS2 无需适配**：原生 target_loads 已存在；且其值为负数编码，语义与本项目的
   zone 百分比不同，不可混用。

## 运行时指纹：scmversion

`/sys/module/sched_walt/scmversion`（Xiaomi/Qualcomm 的 vendor 补丁字段，mainline 只有
srcversion）记录**该模块自身源码树**的 commit 哈希，实测两台机器都与 ko vermagic 的
`-g<hash>` 后缀一致；且与内核镜像的 commit 不同（vendor 模块钉在自己的树上）。
相比 srcversion（两台机器均为空），它在这里是可用的 sched-walt 源码级指纹。

局限：源码级而非二进制级——同 commit 换工具链/编译参数重编，scmversion 不变但结构偏移
可能变。因此运行时判定键用 (机型, fingerprint, scmversion) 三元组，全量 sha256
（dd vendor_boot）留作标定基准与抽查。新机型先跑 dump_device.sh 探明有 scmversion
还是只有 srcversion（探针顺序：scmversion → srcversion → 无，退回 fingerprint+kver）。

## 纵向跟踪（OS2 → OS3 → OS4）

Mix Flip 2（bixi）随系统升级逐版本采集：升级后新开
`compare/bixi-<新版本>/` 重跑 dump，与前一版本 diff（crcs.txt / analysis.txt /
sched-walt.ko 哈希 / scmversion），观察 vendor 侧参数在第几个版本开始漂移、
内核侧 CRC 是否始终不动。重点对比项：

```
diff compare/bixi-A/crcs.txt compare/bixi-B/crcs.txt   # 内核 ABI 漂移
diff compare/bixi-A/analysis.txt compare/bixi-B/analysis.txt
sha256sum compare/bixi-*/ko_out/sched-walt.ko          # vendor WALT 换没换
cat compare/bixi-*/walt_module_scmversion.txt          # 源码树 commit 变没变
```

### 第一站：OS2.0.213 → OS3.1（Android 15 → 16，内核 6.6.56 → 6.6.77）

| 项 | 变化 |
|---|---|
| sched-walt.ko | **换了**：1114056 → 1148416 字节，sha256 不同，scmversion g7eb9096 → gcdef4b3 |
| zone 机制 | **OS3.1 新增 zone_max_util_pct**（OS2 无此机制）——vendor 大版本必然重建 WALT |
| kCFI show/store 哈希 | 不变（0x7fdc93ec / 0x32a83d47） |
| BTF 结构布局 | 六项全同 |
| module_layout CRC | 不变 0x4e276f37 |
| 导入 CRC 表 | 338 共有项中仅 **5 项变化**：mi_dequeue/enqueue_task_fair_func、mi_try_to_wake_up_func、mi_walt_get_indicies_func、wake_render_func——全是 Xiaomi vendor 钩子，非 GKI 内核符号；新增 13 项、消失 1 项。**我们模块导入的符号无一变化** |

结论：跨 HyperOS 大版本，vendor 侧必然重编 sched-walt（机制都会加节点），
内核侧 GKI 符号纹丝不动，漂移只出现在 vendor 自己加的钩子上——进一步支持
"内核侧按代际归档、vendor 侧按 scmversion/fingerprint 逐版本核对"的策略。

### 第二站（终点）：OS4 端点 dada-4.0.19（小米 15）

Flip 2 官方未推送 OS4，纵向链以同厂同代的 dada-4.0.19 收尾
（HyperOS 4.0.19 / Android 17 / 6.6.118）。bixi-3.1 ↔ dada-4.0.19 交叉对比：
299 个共有符号 CRC **零变化**（含 OS2→OS3 时变过的 5 个 vendor 钩子，其新值
延续到 OS4）；两机 WALT 仍不同树（scmversion gcdef4b3 vs gbc807ab，机制节点
集合相同，尺寸不同——折叠屏与直板机的设备差异）。

### 全链总结（OS2 → OS3 → OS4，6.6-android15 代际内）

| 项 | 累计变化 |
|---|---|
| 我们模块导入的内核符号 CRC | **全程零漂移** |
| module_layout / BTF 结构布局 | 全程不变 |
| kCFI show/store 签名哈希 | 全程不变 |
| vendor 钩子符号（mi_* 等） | 仅在 OS2→OS3 变过一次，之后稳定 |
| sched-walt.ko | 每站都不同（scmversion 逐版变化，与 ko 哈希同步） |
| WALT 机制 | OS3 起加入 zone 机制，OS4 保持 |

实验到此收官：内核侧"按 GKI 代际归档"在三站大版本跨度上全程成立；
vendor 侧"按 scmversion 逐版本核对"被证明是**唯一**需要精确匹配的维度。

## 对云端下发设计的落点

- 匹配键：`uname -r` 完整串 + fingerprint；代际信息可从 kver 解析。
- 内核侧校验 = CRC 表比对（构建时已收进 crcs.h），vermagic 重刷作为卫生措施。
- vendor 侧校验 = sched-walt.ko 哈希 + 机制符号清单（zone/target_loads/hispeed 等
  存在性），materials 已留底。
- 404 兜底逻辑不变：机制不符（如原生已有 target_loads）不下发。
