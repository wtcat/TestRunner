/*
 * Copyright 2022 wtcat
 *
 * OS adapt layer for zephyr
 */
#define pr_fmt(fmt) "os_platform: " fmt
#define CONFIG_LOGLEVEL LOGLEVEL_DEBUG
#include <errno.h>
#include <string.h>
#include <init.h>
#include <device.h>
#include <sys/printk.h>
#include <sys/__assert.h>
#include <drivers/flash.h>
#include <partition/partition.h>
#include "board_cfg.h"

#include "basework/malloc.h"
#include "basework/rq.h"
#include "basework/log.h"
#include "basework/crsi.h"
#include "basework/dev/disk.h"
#include "basework/dev/partition.h"

#ifndef CONFIG_BASEWORK_MEM_SIZE
#define CONFIG_BASEWORK_MEM_SIZE (32 * 1024)
#endif

/*
 * Memory allocate implement
 */				
static char mem_buffer[MAX(CONFIG_BASEWORK_MEM_SIZE, Z_HEAP_MIN_SIZE)] __aligned(8);
static STRUCT_SECTION_ITERABLE(k_heap, mem_heap) = {
    .heap = {
        .init_mem = mem_buffer,
        .init_bytes = sizeof(mem_buffer),
    }
};

void *__general_aligned_alloc(size_t alignment, size_t size) {
    void *ptr = k_heap_aligned_alloc(&mem_heap, alignment, size, K_NO_WAIT);
    __ASSERT_NO_MSG(ptr != NULL);
    return ptr;
}

void *__general_aligned_alloc_debug(size_t alignment, size_t size, 
    const char *func, int line) {
    pr_info("# aligned-allocate(%s@%d): %d bytes\n", func, line, size);
    return __general_aligned_alloc(alignment, size);
}

void *__general_malloc(size_t size) {
    void *ptr = k_heap_aligned_alloc(&mem_heap, sizeof(void *), size, K_NO_WAIT);
    __ASSERT_NO_MSG(ptr != NULL);
    return ptr;
}

void *__general_malloc_debug(size_t size, const char *file, int line) {
    pr_info("# allocate(%s@%d): %d bytes\n", file, line, size);
    return __general_malloc(size);
}

void *__general_calloc(size_t n, size_t size) {
    size_t alloc_size = n * size;
    void *ptr = k_heap_aligned_alloc(&mem_heap, sizeof(void *), alloc_size, K_NO_WAIT);
    __ASSERT_NO_MSG(ptr != NULL);
    memset(ptr, 0, alloc_size);
    return ptr;
}

void *__general_calloc_debug(size_t n, size_t size, const char *file, int line) {
    pr_info("# callocate(%s@%d): %d bytes\n", file, line, n*size);
    return __general_calloc(n, size);
}

void  __general_free(void *ptr) {
    k_heap_free(&mem_heap, ptr);
}

void *__general_realloc(void *ptr, size_t size) {
    k_spinlock_key_t key = k_spin_lock(&mem_heap.lock);
    void *p;

    p = sys_heap_aligned_realloc(&mem_heap.heap, ptr, sizeof(void *), size);
    k_spin_unlock(&mem_heap.lock, key);
    return p;
}

void *__general_realloc_debug(void *ptr, size_t size, const char *func, int line) {
    pr_info("# reallocate(%s@%d): %d bytes\n", func, line, size);
    return __general_realloc(ptr, size);
}


/*
 * Log printer register 
 */
static int printk_format(void *context, const char *fmt, va_list ap) {
    (void) context;
    vprintk(fmt, ap);
    return 0;
}

static struct printer log_printer = {
    .format = printk_format
};

struct crsi_key *crsi_get_key(void) {
    static struct crsi_key _crsi_key ;
    static struct printer crsi_printer;

    if (_crsi_key.magic != CRSI_MAGIC) {
        _crsi_key.magic = CRSI_MAGIC;
        queue_format_init(&crsi_printer, _crsi_key.kdump, sizeof(_crsi_key.kdump));
        // __log_crsi_printer = &crsi_printer;
    }
    return &_crsi_key;
}


/*
 * Platform Device
 */
static const char *platform_flash_getname(device_t dev) {
    const struct device *zdev = (const struct device *)dev;
    return zdev->name;
}

static int platform_flash_read(device_t dd, void *buf, size_t size, long offset) {
    return flash_read(dd, offset, buf, size);
}

static int platform_flash_write(device_t dd, const void *buf, size_t size, long offset) {
    return flash_write(dd, offset, buf, size);
}

static int platform_flash_erase(device_t dd, long offset, size_t size) {
    return flash_erase(dd, offset, size);
}

static int platform_flash_ioctl(device_t dd, long cmd, void *arg) {
    (void) dd;
    (void) cmd;
    (void) arg;
    return -ENOSYS;
}

static int platform_flash_init(void) {
    const struct flash_pages_layout *layout;
    const struct flash_driver_api *api;
    static struct disk_device *dd;
    const struct device *zdev;
    const char *devnames[] = {
        CONFIG_SPI_FLASH_NAME,
        CONFIG_SPI_FLASH_1_NAME,
        CONFIG_SPI_FLASH_2_NAME
    };
    
    for (int i = 0; i < ARRAY_SIZE(devnames); i++) {
        zdev = device_get_binding(devnames[i]);
        if (zdev) {
            api = (const struct flash_driver_api *)zdev->api;
            __ASSERT_NO_MSG(api->page_layout != NULL);
            api->page_layout(zdev, &layout, NULL);

            dd = general_calloc(1, sizeof(*dd));
            if (dd == NULL) {
                pr_err("No more memory\n");
                return -ENOMEM;
            }

            dd->dev = (void *)zdev;
            dd->addr = 0;
            dd->blk_size = layout->pages_size;
            dd->len = layout->pages_count * layout->pages_size;
            dd->read = platform_flash_read;
            dd->write = platform_flash_write;
            dd->erase = platform_flash_erase;
            dd->ioctl = platform_flash_ioctl;
            dd->get_name = platform_flash_getname;
            int err = disk_device_register(dd);
            if (err) {
                pr_err("Register disk device failed: %d\n", err);
                return err;
            }
        }
    }

    return -EINVAL;
}

/*
 * Partition table
 */
struct pt_arg {
    char name[16];
    const char *parent;
    long offset;
    size_t len;
};

extern void partition_iterate(
	void (*iterator)(const char *name, uint32_t ofs, size_t size, int storage_id, void *arg), 
	void *arg);

static void __unused partition_iterator(const char *name, uint32_t ofs, size_t size, 
    int storage_id, void *arg) {
    struct pt_arg *pt = (struct pt_arg *)arg;

    if (strcmp(name, "USER"))
        return;
    memcpy((void *)pt->name, name, 8);
    pt->name[8] = '\0';
    pt->offset = ofs;
    pt->len = size;
    switch (storage_id) {
    case STORAGE_ID_NOR:
        pt->parent = CONFIG_SPI_FLASH_NAME;
        break;
    case STORAGE_ID_DATA_NOR:
        pt->parent = CONFIG_SPI_FLASH_2_NAME;
        break;
    case STORAGE_ID_SD:
        pt->parent = "sd";
        break;
    case STORAGE_ID_NAND:
        pt->parent = "spinand";
        break;
    default:
        pr_err("invalid partition (%s)\n", name);
        pt->parent = NULL;
        break; 
    }
}

static int __unused platform_partition_init(void) {
    struct pt_arg ptarg = {0};

    partition_iterate(partition_iterator, &ptarg);
    if (ptarg.parent == NULL)
        return -EINVAL;

    return partitions_configure_build(ptarg.offset, ptarg.len, ptarg.parent);
}


/*
 * Platform Initialization
 */
static int os_early_platform_init(const struct device *dev __unused) {
    pr_log_init(&log_printer);
    crsi_get_key();
    return 0;
}
SYS_INIT(os_early_platform_init, PRE_KERNEL_2, 10);

static int os_platform_init(const struct device *dev __unused) {
    static struct printer disk_printer;
    platform_flash_init();
    platform_partition_init();
    disklog_format_init(&disk_printer);
    pr_disklog_init(&disk_printer);
    return 0;
}
SYS_INIT(os_platform_init, APPLICATION, 0);