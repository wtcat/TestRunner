/*
 * Copyright 2022 wtcat
 *
 * File system implement for zephyr
 */
#include <errno.h>
#include <assert.h>
#include <init.h>
#include <fs/fs.h>

#include "basework/os/osapi_config.h"
#include "basework/os/osapi_fs.h"
#include "basework/os/osapi_timer.h"
#include "basework/os/osapi_obj.h"
#include "basework/log.h"

struct file {
    struct file_base base;
    struct fs_file_t zfp;
};

static int zephyr_fs_open(os_file_t fd, const char *path, int flags) {
    struct file *fp = (struct file *)fd;
    int orw = flags & VFS_O_MASK;
    int ff = 0;

    switch (orw) {
    case VFS_O_RDONLY:
        ff |= FS_O_READ;
        break;
    case VFS_O_WRONLY:
        ff |= FS_O_WRITE;
        break;
    case VFS_O_RDWR:
        ff |= FS_O_RDWR;
        break;
    default:
        return -EINVAL;
    }
    if (orw & VFS_O_APPEND)
        ff |= FS_O_APPEND;
    if (orw & VFS_O_CREAT)
        ff |= FS_O_CREATE;

    fs_file_t_init(&fp->zfp);
    return fs_open(&fp->zfp, path, ff);
}

static int zephyr_fs_close(os_file_t fd) {
    struct file *fp = (struct file *)fd;
    return fs_close(&fp->zfp);
}

static int zephyr_fs_ioctl(os_file_t fd, int cmd, void *args) {
    (void) fd;
    (void) cmd;
    (void) args;
    return -ENOSYS;
}

static int zephyr_fs_read(os_file_t fd, void *buf, size_t len) {
    struct file *fp = (struct file *)fd;
    return fs_read(&fp->zfp, buf, len);
}

static int zephyr_fs_write(os_file_t fd, const void *buf, size_t len) {
    struct file *fp = (struct file *)fd;
    return fs_write(&fp->zfp, buf, len);
}

static int zephyr_fs_flush(os_file_t fd) {
    struct file *fp = (struct file *)fd;
    return fs_sync(&fp->zfp);
}

static int zephyr_fs_lseek(os_file_t fd, off_t offset, int whence) {
    struct file *fp = (struct file *)fd;
    return fs_seek(&fp->zfp, offset, whence);
}

static int zephyr_fs_rename(os_filesystem_t fs, const char *oldpath, 
    const char *newpath) {
    (void) fs;
    return fs_rename(oldpath, newpath);
}

static int zephyr_fs_ftruncate(os_file_t fd, off_t length) {
    struct file *fp = (struct file *)fd;
    return fs_truncate(&fp->zfp, length);
}

static int zephyr_fs_unlink(os_filesystem_t fs, const char *path) {
    (void) fs;
    return fs_unlink(path);
}

static int zephyr_fs_mkdir(os_filesystem_t fs, const char *path) {
    (void) fs;
    return fs_mkdir(path);
}

static int zephyr_fs_stat(os_filesystem_t fs, const char *path, 
    struct vfs_stat *buf) {
    struct fs_dirent stat;
    int err;

    err = fs_stat(path, &stat);
    if (!err) {
        buf->st_blksize = 0;
        buf->st_blocks = 0;
        buf->st_size = stat.size;
    }

    return err;
}

static struct file os_files[CONFIG_OS_MAX_FILES];
struct file_class zephyr_file_class = {
    .open = zephyr_fs_open,
    .close = zephyr_fs_close,
    .ioctl = zephyr_fs_ioctl,
    .read = zephyr_fs_read,
    .write = zephyr_fs_write,
    .flush = zephyr_fs_flush,
    .lseek = zephyr_fs_lseek,
    .truncate = zephyr_fs_ftruncate,

    .mkdir = zephyr_fs_mkdir,
    .unlink = zephyr_fs_unlink,
    .stat = zephyr_fs_stat,
    .rename = zephyr_fs_rename,
    .robj = OBJ_RES_INIT(os_files)
};

static int zephyr_fs_init(const struct device *dev __unused) {
    return vfs_register(&zephyr_file_class);
}

SYS_INIT(zephyr_fs_init, APPLICATION, 0);
