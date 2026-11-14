# 如何判断 KernelSU 越狱模式（临时 root）

记录 KernelSU「越狱模式 / Jailbreak mode」（即临时 root）的判定原理与检测方法。
结论基于 KernelSU 源码（本仓库 `KernelSU/` 克隆），关键位置均给出文件与行号。

## 1. 三态与越狱模式的定义

KernelSU 按加载方式分三种运行态（`userspace/ksud/src/ksucalls.rs:195` `runtime_mode()`）：

| 运行态 | 含义 | 持久性 |
|---|---|---|
| `built-in` | KSU 编进内核（GKI / boot 镜像补丁） | 永久，重启保留 |
| `lkm` | KSU 以内核模块形式，开机由 init 加载 | 永久（模块装进 boot） |
| `late-load` | KSU 模块在开机后由用户态进程 insmod 加载 | **临时，重启丢失** |

**越狱模式 = `late-load`**。管理器在该态显示「Jailbreak mode」，并警告重启会丢 root
（`manager/app/src/main/res/values/strings.xml:104`）。root 靠漏洞提权（Magica）取得，
内核本身未被持久修改。

注意区分 `lkm` 与 `late-load`：两者都是模块、都带 KSU 内核符号，但 `lkm` 是开机由
init 加载的永久态，`late-load` 才是重启即失的越狱态。**不能靠「有没有 KSU 符号」区分。**

## 2. 判定原理

判定的根在内核初始化时的一行（`kernel/core/init.c:115`）：

```c
#ifdef MODULE
    ksu_late_loaded = (current->pid != 1);
#else
    ksu_late_loaded = false;
#endif
```

- KSU 编进内核（非 MODULE）：`ksu_late_loaded` 恒为 false。
- KSU 是模块（MODULE）：看加载它的进程。由 init（pid 1）开机加载 → false；
  由用户态进程（pid ≠ 1，即漏洞提权后 insmod）加载 → true。

内核在 `get_info` 应答里上报标志（`kernel/supercall/dispatch.c:46`）：

```c
#ifdef MODULE
    cmd.flags |= KSU_GET_INFO_FLAG_LKM;        /* 是模块 */
#endif
    if (ksu_late_loaded)
        cmd.flags |= KSU_GET_INFO_FLAG_LATE_LOAD;   /* 晚加载 = 越狱 */
```

用户态读这个 flags 即可判定。整条链路：

```
kernel/core/init.c:116        ksu_late_loaded = (current->pid != 1)
        ↓
kernel/supercall/dispatch.c:60  get_info 置 FLAG_LATE_LOAD
        ↓  (supercall IOCTL)
userspace/ksud/src/ksucalls.rs:178  is_late_load() = flags & FLAG_LATE_LOAD
        ↓
manager/.../HomeMiuix.kt:255    isLateLoadMode → 显示 "[Jailbreak mode]"
```

## 3. 检测方法

### 3.1 KSU 模块脚本（最简单）

ksud 执行模块脚本（post-fs-data.sh / service.sh 等）时注入环境变量
（`userspace/ksud/src/module.rs:63` `get_common_script_envs`）：

- `KSU_RUNTIME_MODE` = `late-load` / `lkm` / `built-in`
- `KSU_LATE_LOAD` = `1`（仅越狱态置位，`module.rs:90`）
- 另有 `KSU`、`KSU_KERNEL_VER_CODE`、`KSU_VER`、`KSU_UAPI_VER` 等

脚本里直接判断：

```sh
if [ "$KSU_LATE_LOAD" = "1" ]; then
    echo "临时 root（越狱），重启丢，应走热重启"
else
    echo "永久 root"
fi
# 或： [ "$KSU_RUNTIME_MODE" = "late-load" ]
```

walt_ext 的 service.sh 区分临时/永久 root 用这个即可，无需碰内核接口。

### 3.2 任意 ELF 程序 / 非模块脚本（supercall IOCTL）

ksud 的取法（`userspace/ksud/src/ksucalls.rs`）：先拿 driver FD，再 ioctl GET_INFO，
查 flags 的 bit2。

FD 两条路（`ksucalls.rs:88` `scan_driver_fd` / `ksucalls.rs:120` `init_driver_fd`）：
1. 扫 `/proc/self/fd`，找 readlink 目标为 `anon_inode:[ksu_driver]` 或
   `anon_inode:[ksu_driver_su]` 的 FD（继承来的）。
2. 没有则用 reboot 魔数 syscall 安装：`syscall(SYS_reboot, 0xDEADBEEF, 0xCAFEBABE, 0, &fd)`，
   KSU 的 reboot kprobe hook 会回填一个 driver FD。

C 示例：

```c
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>

struct ksu_get_info { uint32_t version, flags, features, uapi_version; };
#define KSU_IOCTL_GET_INFO  _IOR('K', 2, struct ksu_get_info)  /* = 0x80104B02 */
#define FLAG_LKM        (1u << 0)
#define FLAG_LATE_LOAD  (1u << 2)

static int get_ksu_fd(void) {
    DIR *d = opendir("/proc/self/fd");
    if (d) {
        struct dirent *e; char path[64], link[128];
        while ((e = readdir(d))) {
            int n = atoi(e->d_name);
            snprintf(path, sizeof path, "/proc/self/fd/%d", n);
            ssize_t r = readlink(path, link, sizeof link - 1);
            if (r > 0) {
                link[r] = 0;
                if (!strcmp(link, "anon_inode:[ksu_driver]") ||
                    !strcmp(link, "anon_inode:[ksu_driver_su]")) {
                    closedir(d); return n;
                }
            }
        }
        closedir(d);
    }
    int fd = -1;
    syscall(SYS_reboot, 0xDEADBEEF, 0xCAFEBABE, 0, &fd);
    return fd;
}

int main(void) {
    int fd = get_ksu_fd();
    if (fd < 0) { puts("no KSU driver"); return 1; }
    struct ksu_get_info info = {0};
    if (ioctl(fd, KSU_IOCTL_GET_INFO, &info) < 0) { perror("ioctl"); return 1; }
    if (info.flags & FLAG_LATE_LOAD)      puts("temporary (jailbreak)");
    else if (info.flags & FLAG_LKM)       puts("lkm (permanent module)");
    else                                  puts("built-in (permanent)");
    return 0;
}
```

脚本若不想写 C，可把上面编译成小工具调用，或在已是 KSU 模块上下文时直接读 3.1 的环境变量。

### 3.3 管理器 UI

KSU 管理器首页在 `isLateLoadMode` 时追加「[Jailbreak mode]」
（`manager/.../HomeMiuix.kt:255`）。人工确认最直接。

## 4. uapi 常量参考（`uapi/supercall.h`）

```c
KSU_INSTALL_MAGIC1 = 0xDEADBEEF        /* reboot hook 安装 FD 魔数 */
KSU_INSTALL_MAGIC2 = 0xCAFEBABE

KSU_GET_INFO_FLAG_LKM       = 1 << 0   /* 0x1，模块形式 */
KSU_GET_INFO_FLAG_MANAGER   = 1 << 1   /* 0x2 */
KSU_GET_INFO_FLAG_LATE_LOAD = 1 << 2   /* 0x4，越狱/临时 */
KSU_GET_INFO_FLAG_PR_BUILD  = 1 << 3   /* 0x8 */
KSU_GET_INFO_FLAG_BUNDLED   = 1 << 4   /* 0x10 */
KSU_GET_INFO_FLAG_PREINIT   = 1 << 5   /* 0x20 */

struct ksu_get_info_cmd {              /* 16 字节 */
    __u32 version;
    __u32 flags;
    __u32 features;
    __u32 uapi_version;
};

KSU_IOCTL_GET_INFO        = _IOR('K', 2, struct ksu_get_info_cmd)   /* 0x80104B02 */
KSU_IOCTL_GET_INFO_LEGACY = _IOC(_IOC_READ, 'K', 2, 0)              /* 旧版兜底 */
```

## 5. 注意事项

- **判据是 LATE_LOAD 位（bit2），不是 LKM 位**。lkm 但非 late-load 是永久态。
- **需要 root 权限**：reboot 魔数 syscall 与 FD 获取都需权限。检测 root 类型时通常已有 root。
- **符号法不可靠**：lkm 与 late-load 都有 KSU 内核符号，`grep ksu /proc/kallsyms` 区分不了。
- **版本差异**：不同 KSU 版本 IOCTL 号/结构可能微调，ksud 用 GET_INFO_LEGACY 兜底，并按
  `uapi_version` 校验（`ksucalls.rs:205`）。跨版本集成最好校验 uapi_version。
- **软重启保留越狱**：越狱态优先软重启（`manager/.../SettingsRepositoryImpl.kt:25`），
  软重启保留内核与临时 root；冷重启丢 root。

## 6. 实测验证

用第 3.2 节的 C 工具（NDK/aarch64-linux-gnu-gcc 静态编译）实机运行：

| 设备 | 内核 | get_info 应答 | 判定 |
|---|---|---|---|
| Xiaomi 25042PN24C（HyperOS 3） | 6.6.118-android15-8 | version=32525, flags=0x5, uapi=2 | late-load（越狱）✓ |

flags=0x5 = LKM(bit0) + LATE_LOAD(bit2)，工具正确报 temporary/jailbreak，与该机会话
实际为越狱态一致。

**重要教训：「重启后 root 是否保留」不是可靠判据。** 越狱态若开了管理器「自动越狱」
（`settings_auto_jailbreak`，开机检测到 Permissive SELinux 时用 Magica 自动重新提权），
重启后 root 会被自动恢复，看起来像永久，但 get_info 仍是 LATE_LOAD。判定运行态必须以
get_info 的 LATE_LOAD 位（或模块脚本的 KSU_LATE_LOAD 环境变量）为准，不能靠重启现象推断。

临时 root（late-load）机型的 walt_tl 持久化须走热重启，不能冷重启。