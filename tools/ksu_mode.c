#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>

struct ksu_get_info { uint32_t version, flags, features, uapi_version; };
#define KSU_IOCTL_GET_INFO  _IOR('K', 2, struct ksu_get_info)
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
    if (fd < 0) { puts("no KSU driver fd"); return 1; }
    struct ksu_get_info info = {0};
    if (ioctl(fd, KSU_IOCTL_GET_INFO, &info) < 0) { perror("ioctl GET_INFO"); return 1; }
    printf("fd=%d version=%u flags=0x%x features=%u uapi_version=%u\n",
           fd, info.version, info.flags, info.features, info.uapi_version);
    if (info.flags & FLAG_LATE_LOAD)      puts("MODE: temporary (jailbreak / late-load)");
    else if (info.flags & FLAG_LKM)       puts("MODE: lkm (permanent module)");
    else                                  puts("MODE: built-in (permanent)");
    return 0;
}