/*
 * Copyright 2022 wtcat
 */
#define pr_fmt(fmt) "osapi_fs: "fmt
#include <assert.h>

#include "basework/os/osapi_config.h"
#include "basework/os/osapi_fs.h"
#include "basework/os/osapi.h"
#include "basework/log.h"


static struct file_class *_vfs;

// static inline int file_acquire(struct file_base *fbase) {
//     return 0;
// }

// static inline void file_release(struct file_base *fbase) {
    
// }

int vfs_open(os_file_t *fd, const char *path, int flags) {
    struct file_class *f;
    struct file_base *fp;
    int err;

    if (fd == NULL || path == NULL)
        return -EINVAL;

    if (_vfs == NULL)
        return -EINVAL;
    f = _vfs;

    if (!os_obj_ready(OBJ_FILE_CLASS)) {
        int err = os_obj_initialize(OBJ_FILE_CLASS, f->robj.start, 
            f->robj.size, f->robj.size/CONFIG_OS_MAX_FILES);
        if (err) {
            pr_err("Initialize timer object failed (%d)\n", err);
            return err;
        }
    }

    fp = os_obj_allocate(OBJ_FILE_CLASS);
    assert(fp != NULL);

    fp->f_class = _vfs;
    assert(fp->f_class->open != NULL);

    err = fp->f_class->open(fp, path, flags);
    if (err) {
        pr_err("Open file failed(%d)\n", err);
        os_obj_free(OBJ_FILE_CLASS, fp);
        return err;
    }

    *fd = fp;
    return 0;
}

int vfs_close(os_file_t fd) {
    struct file_base *fp = (struct file_base *)fd;
    int err;

    assert(fp != NULL);
    assert(fp->f_class != NULL);
    assert(fp->f_class->close != NULL);

    err = fp->f_class->close(fd);
    if (err) {
        pr_err("File close failed (%d)\n", err);
        return err;
    }

    os_obj_free(OBJ_FILE_CLASS, fd);
    return 0;
}

int vfs_ioctl(os_file_t fd, int cmd, void *args) {
    struct file_base *fp = (struct file_base *)fd;

    assert(fp != NULL);
    assert(fp->f_class != NULL);
    assert(fp->f_class->ioctl != NULL);

    return fp->f_class->ioctl(fd, cmd, args);
}

int vfs_read(os_file_t fd, void *buf, size_t len) {
    struct file_base *fp = (struct file_base *)fd;
    assert(fp != NULL);
    assert(fp->f_class != NULL);
    assert(fp->f_class->read != NULL);

    if (buf == NULL)
        return -EINVAL;
    if (len == 0)
        return 0;

    return fp->f_class->read(fd, buf, len);
}

int vfs_write(os_file_t fd, const void *buf, size_t len) {
    struct file_base *fp = (struct file_base *)fd;
    assert(fp != NULL);

    if (buf == NULL)
        return -EINVAL;
    if (len == 0)
        return 0;

    assert(fp->f_class != NULL);
    assert(fp->f_class->write != NULL);

    return fp->f_class->write(fd, buf, len);
}

int vfs_flush(os_file_t fd) {
    struct file_base *fp = (struct file_base *)fd;
    assert(fp != NULL);
    assert(fp->f_class != NULL);
    assert(fp->f_class->flush != NULL);

    return fp->f_class->flush(fd);
}

int vfs_lseek(os_file_t fd, off_t offset, int whence) {
    struct file_base *fp = (struct file_base *)fd;
    assert(fp != NULL);
    assert(fp->f_class != NULL);
    assert(fp->f_class->lseek != NULL);

    return fp->f_class->lseek(fd, offset, whence);
}

int vfs_getdents(os_file_t fd, struct dirent *dirp, size_t nbytes) {
    struct file_base *fp = (struct file_base *)fd;
    assert(fp != NULL);
    if (dirp == NULL)
        return -EINVAL;

    assert(fp->f_class != NULL);
    assert(fp->f_class->getdents != NULL);

    return fp->f_class->getdents(fd, dirp, nbytes);
}

int vfs_ftruncate(os_file_t fd, off_t length) {
    struct file_base *fp = (struct file_base *)fd;
    assert(fp != NULL);
    assert(fp->f_class != NULL);
    assert(fp->f_class->truncate != NULL);

    return fp->f_class->truncate(fd, length);
}

int vfs_rename(const char *oldpath, const char *newpath) {
    struct file_class *fp = _vfs;
    if (oldpath == NULL || newpath == NULL)
        return -EINVAL;

    assert(fp != NULL);
    assert(fp->rename != NULL);

    return fp->rename(NULL, oldpath, newpath);
}

int vfs_unlink(const char *path) {
    struct file_class *fp = _vfs;
    if (path == NULL)
        return -EINVAL;

    assert(fp != NULL);
    assert(fp->unlink != NULL);

    return fp->unlink(NULL, path);
}

int vfs_stat(const char *path, struct vfs_stat *buf) {
    struct file_class *fp = _vfs;
    if (path == NULL)
        return -EINVAL;

    assert(fp != NULL);
    assert(fp->stat != NULL);

    return fp->stat(NULL, path, buf);
}

int vfs_register(struct file_class *fs) {
    if (!fs)
        return -EINVAL;
    if (_vfs)
        return -EBUSY;
    _vfs = fs;
    return 0;
}