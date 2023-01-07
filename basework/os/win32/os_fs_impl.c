/*
 * Copyright 2022 wtcat
 */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "basework/os/osapi_config.h"
#include "basework/os/osapi_fs.h"
#include "basework/log.h"

struct file {
    struct file_base base;
    FILE *fp;
};

static int cstd_fs_open(os_file_t fd, const char *path, int flags) {
    struct file *fp = (struct file *)fd;
    int orw = flags & VFS_O_MASK;
    char mode[4] = {0};

    switch (orw) {
    case VFS_O_RDONLY:
        strcpy(mode, "rb");
        break;
    case VFS_O_WRONLY:
        strcpy(mode, "wb");
        if (orw & VFS_O_APPEND)
            strcpy(mode, "ab");
        break;
    case VFS_O_RDWR:
        strcpy(mode, "rb+");
        if (orw & VFS_O_APPEND)
            strcpy(mode, "ab+");
        break;
    default:
        return -EINVAL;
    }

    fp->fp = fopen(path, mode);
    if (!fp->fp)
        return -ENXIO;
    return 0;
}

static int cstd_fs_close(os_file_t fd) {
    struct file *fp = (struct file *)fd;
    return fclose(fp->fp);
}

static int cstd_fs_ioctl(os_file_t fd, int cmd, void *args) {
    (void) fd;
    (void) cmd;
    (void) args;
    return -ENOSYS;
}

static int cstd_fs_read(os_file_t fd, void *buf, size_t len) {
    struct file *fp = (struct file *)fd;
    size_t size;

    size = fread(buf, len, 1, fp->fp);
    if (len == 0)
        return -EIO;
    return size;
}

static int cstd_fs_write(os_file_t fd, const void *buf, size_t len) {
    struct file *fp = (struct file *)fd;
    size_t size;

    size = fwrite(buf, len, 1, fp->fp);
    if (size < len)
        return -EIO;
    return size;
}

static int cstd_fs_flush(os_file_t fd) {
    struct file *fp = (struct file *)fd;
    return fflush(fp->fp);
}

static int cstd_fs_lseek(os_file_t fd, off_t offset, int whence) {
    struct file *fp = (struct file *)fd;
    return fseek(fp->fp, offset, whence);
}

static int cstd_fs_rename(os_filesystem_t fs, const char *oldpath, 
    const char *newpath) {
    (void) fs;
    (void) oldpath;
    (void) newpath;
    return -ENOSYS;
}

static int cstd_fs_ftruncate(os_file_t fd, off_t length) {
    (void) fd;
    (void) length;
    return -ENOSYS;
}

static int cstd_fs_unlink(os_filesystem_t fs, const char *path) {
    (void) fs;
    (void) path;
    return -ENOSYS;
}

static struct file os_files[CONFIG_OS_MAX_FILES];
struct file_class cstd_file_class = {
    .open = cstd_fs_open,
    .close = cstd_fs_close,
    .ioctl = cstd_fs_ioctl,
    .read = cstd_fs_read,
    .write = cstd_fs_write,
    .flush = cstd_fs_flush,
    .lseek = cstd_fs_lseek,
    .truncate = cstd_fs_ftruncate,

    .mkdir = NULL,
    .unlink = cstd_fs_unlink,
    .stat = NULL,
    .rename = cstd_fs_rename,
    .robj = OBJ_RES_INIT(os_files)
};

int cstd_fs_init(void) {
    return vfs_register(&cstd_file_class);
}
