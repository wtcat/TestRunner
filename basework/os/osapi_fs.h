/*
 * Copyright 2022 wtcat
 *
 * Borrowed from rt-thread
 */
#ifndef BASEWORK_OS_OSAPI_FS_H_
#define BASEWORK_OS_OSAPI_FS_H_

#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <assert.h>
#include <sys/types.h>

#include "basework/os/osapi_obj.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef void *os_filesystem_t;
typedef void *os_file_t;

struct stat;
struct dirent;

#define VFS_O_RDONLY 0
#define VFS_O_WRONLY 1
#define VFS_O_RDWR   2
#define VFS_O_MASK   3

#define VFS_O_CREAT  0x0200
#define VFS_O_APPEND 0x0008


#define VFS_SEEK_SET        0  /* Seek from beginning of file.  */
#define VFS_SEEK_CUR        1  /* Seek from current position.  */
#define VFS_SEEK_END        2  /* Set file pointer to EOF plus "offset" */

struct vfs_stat {
	unsigned long st_size; /* File size */
	unsigned long st_blksize;
	unsigned long st_blocks;
};

struct file_class {
    /* File operations */
    int (*open)     (os_file_t fd, const char *path, int flags);
    int (*close)    (os_file_t fd);
    int (*ioctl)    (os_file_t fd, int cmd, void *args);
    int (*read)     (os_file_t fd, void *buf, size_t count);
    int (*write)    (os_file_t fd, const void *buf, size_t count);
    int (*flush)    (os_file_t fd);
    int (*lseek)    (os_file_t fd, off_t offset, int whence);
    int (*getdents) (os_file_t fd, struct dirent *dirp, uint32_t count);
    int (*truncate) (os_file_t fd, off_t length);
    // int (*poll)     (os_file_t *fd, struct rt_pollreq *req);

    /* filesystem operations */
    int (*mkdir)    (os_filesystem_t fs, const char *pathname);
    int (*unlink)   (os_filesystem_t fs, const char *pathname);
    int (*stat)     (os_filesystem_t fs, const char *filename, struct vfs_stat *buf);
    int (*rename)   (os_filesystem_t fs, const char *oldpath, const char *newpath);

    struct obj_resource robj;
};

struct file_base {
    struct file_class *f_class;
    // unsigned int flags;
};

/*
 * File interface
 */
int vfs_open(os_file_t *fd, const char *path, int flags);
int vfs_close(os_file_t fd);
int vfs_ioctl(os_file_t fd, int cmd, void *args);
int vfs_read(os_file_t fd, void *buf, size_t len);
int vfs_write(os_file_t fd, const void *buf, size_t len);
int vfs_flush(os_file_t fd);
int vfs_lseek(os_file_t fd, off_t offset, int whence);
int vfs_getdents(os_file_t fd, struct dirent *dirp, size_t nbytes);
int vfs_ftruncate(os_file_t fd, off_t length);
int vfs_stat(const char *path, struct vfs_stat *buf);

int vfs_register(struct file_class *fs);

#ifdef __cplusplus
}
#endif
#endif /* BASEWORK_OS_OSAPI_FS_H_ */