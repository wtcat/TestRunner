/*
 * Copyright 2022 wtcat
 *
 * Environment variables:
 * 
 * @crash: yes/or (whether the system has occurred exception)
 * @runtime: $time (last running time of the system)
 * @crash-cnt: $times (crash count of the system)
 * @ota_update: Notice that firmware enter OTA update mode
 *
 */
#define pr_fmt(fmt) "<bootloader>: "fmt
#define CONFIG_LOGLEVEL LOGLEVEL_DEBUG

#include <errno.h>
#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "basework/boot/boot.h"
#include "basework/dev/disk.h"
#include "basework/system.h"
#include "basework/env.h"
#include "basework/log.h"
#include "basework/minmax.h"
#include "basework/system.h"


#define REBOOT_MAX_LIMIT 10
#define MIN_TIME 60  /* Unit: seconds */

struct fw_desc {
    struct disk_device *dd;
    long offset;
};

struct indicate_obj {
    const char *action;
    void (*notify)(const char *, int);
    int progress;
};

static struct fw_desc fw_loader, fw_runner, fw_curinfo;
static uint8_t fw_cache[FW_FLASH_BLKSIZE];


static uint32_t crc32_update(uint32_t crc, const uint8_t *data, 
    size_t len) {
	/* crc table generated from polynomial 0xedb88320 */
	static const uint32_t table[16] = {
		0x00000000U, 0x1db71064U, 0x3b6e20c8U, 0x26d930acU,
		0x76dc4190U, 0x6b6b51f4U, 0x4db26158U, 0x5005713cU,
		0xedb88320U, 0xf00f9344U, 0xd6d6a3e8U, 0xcb61b38cU,
		0x9b64c2b0U, 0x86d3d2d4U, 0xa00ae278U, 0xbdbdf21cU,
	};

	crc = ~crc;
	for (size_t i = 0; i < len; i++) {
		uint8_t byte = data[i];
		crc = (crc >> 4) ^ table[(crc ^ byte) & 0x0f];
		crc = (crc >> 4) ^ table[(crc ^ ((uint32_t)byte >> 4)) & 0x0f];
	}
	return ~crc;
}

static bool is_fw_valid(const struct fw_desc *fd, struct firmware_header *outfw) {
    struct firmware_header fw;
    int ret;

    ret = disk_device_read(fd->dd, &fw, sizeof(fw), fd->offset);
    if (ret) {
        pr_err("Read firmware failed(%d)\n", ret);
        return ret;
    }

    if (fw.fh_magic != FH_MAGIC) {
        pr_err("Invalid fireware header (%x)\n", fw.fh_magic);
        return false;
    }

    if (fw.fh_size > fw.fh_isize) {
        pr_err("Invalid fireware size(cur(0x%x) org(0x%x))\n", fw.fh_size, fw.fh_isize);
        return false;
    }

    size_t size = fw.fh_isize;
    size_t offset = fd->offset + sizeof(fw);
    uint32_t crc = 0;
    while (size > 0) {
        size_t bytes = min(size, sizeof(fw_cache));
        ret = disk_device_read(fd->dd, fw_cache, bytes, offset);
        if (ret) 
            return ret;
        crc = crc32_update(crc, fw_cache, bytes);
        offset += bytes;
        size -= bytes;
    }
    if (crc != fw.fh_dcrc) {
        pr_err("CRC verify failed");
        return false;
    }

    if (outfw)
        *outfw = fw;

    return true;
}

static void ota_progress_indicate(struct indicate_obj *obj, int progress) {
    if (obj->notify && 
        progress != obj->progress) {
        obj->progress = progress;
        obj->notify(obj->action, progress);
    }
}

static bool is_fw_runtime_okay(void) {
    if (env_getul("runtime") > 3600)
        return true;
    return false;
}

static bool is_fw_need_recovery(void) {
    if (env_streq("crash", "yes")) {
        unsigned long errs = env_getul("crash-cnt");
        if (errs > REBOOT_MAX_LIMIT)
            return true;

        if (env_getul("runtime") < MIN_TIME)
            env_setint("crash-cnt", errs);
    }

    pr_dbg("The firmware not need recovery mode!\n");
    return false;
}

static bool is_fw_need_update(void) {
    struct firmware_pointer curr, loader;
    int ret;
    
    ret = disk_device_read(fw_loader.dd, &loader, sizeof(loader), fw_loader.offset);
    if (ret) {
        pr_err("read download firmware information failed(%d)\n", ret);
        return false;
    }

    if (loader.fh_magic != FH_MAGIC) {
        pr_warn("download firmware is invalid\n");
        return false;
    }

    ret = disk_device_read(fw_curinfo.dd, &curr, sizeof(curr), fw_curinfo.offset);
    if (ret) {
        pr_err("read current firmware information failed(%d)\n", ret);
        return false;
    }
    
    /*
     * Compare device id and checksum of the firmware
     */
    if (curr.fh_magic == FH_MAGIC) {
        if (loader.fh_magic == curr.fh_magic &&
            curr.fh_devid == loader.fh_devid &&
            curr.fh_dcrc != loader.fh_dcrc)
            return true;
    } else {
        if (loader.fh_magic == FH_MAGIC)
            return true;
    }

    pr_dbg("The firmware don't need to update\n");
    return false;
}

static int fw_save_header(struct fw_desc *fd, const struct firmware_pointer *fp) {
    int ret;

    ret = disk_device_read(fd->dd, fw_cache, 
        sizeof(fw_cache), fd->offset);
    if (ret) {
        pr_err("<%s>Read disk failed(%d)\n", __func__, ret);
        return ret;
    }

    ret = disk_device_erase(fd->dd, fd->offset, sizeof(fw_cache));
    if (ret) {
        pr_err("<%s>Erase disk failed(%d)\n", __func__, ret);
        return ret;
    }

    /* Save current firmware information */
    memcpy(fw_cache, fp, sizeof(*fp));
    ret = disk_device_write(fd->dd, fw_cache, sizeof(fw_cache), fd->offset);
    if (ret) {
        pr_err("<%s> Write file header failed(%d)\n", __func__, ret);
        return ret;
    }

    return 0;
}

/*
 * Copy firmware to run region
 */
static int fw_copy_decomp(const struct fw_desc *dst, 
    const struct fw_desc *src, void (*notify)(const char *, int), 
    const char *action) {
    struct firmware_header fw = {0};
    struct indicate_obj phase = {0};
    int ret;

    if (!is_fw_valid(src, &fw))
        return -EINVAL;

    phase.notify = notify;
    phase.action = action;
    /*
     * Copy firmware data 
     */
    size_t src_ofs = src->offset + sizeof(struct firmware_header);
    size_t fw_size = fw.fh_isize;
    size_t ofs = 0;

    while (fw_size > 0) {
        size_t bytes = min(fw_size, sizeof(fw_cache));
        ret = disk_device_read(src->dd, fw_cache, bytes, src_ofs);
        if (ret) {
            pr_err("<%s>Read disk address(0x%x) failed", __func__, src_ofs);
            return ret;
        }

        if (disk_device_erase(dst->dd, dst->offset + ofs, sizeof(fw_cache))) {
            pr_err("<%s>Erase disk failed\n", __func__);
            return -EIO;
        }
  
        ret = disk_device_write(dst->dd, fw_cache, bytes, dst->offset + ofs);
        if (ret) {
            pr_err("<%s>Write disk address(0x%x) failed", __func__, ofs);
            return ret;
        }

        ofs += bytes;
        src_ofs += bytes;
        fw_size -= bytes;

        ota_progress_indicate(&phase, (ofs * 100) / (fw.fh_isize << 1));
    }

    /* Verify */
    uint32_t crc = 0;
    fw_size = fw.fh_isize;
    ofs = 0;
    while (fw_size > 0) {
        size_t bytes = min(fw_size, sizeof(fw_cache));
        ret = disk_device_read(dst->dd, fw_cache, bytes, dst->offset + ofs);
        if (ret) {
            pr_err("<%s>Read address(0x%x) failed", __func__, ofs);
            break;
        }

        crc = crc32_update(crc, fw_cache, bytes);
        ofs += bytes;
        fw_size -= bytes;

        if (fw_size == 0) {
            if (crc != fw.fh_dcrc) {
                pr_err("CRC veriry failed(cur(0x%x) org(0x%x))\n", crc, fw.fh_dcrc);
                return -EBADF;
            }

            /* Save current firmware information */
            ret = fw_save_header(&fw_curinfo, (struct firmware_pointer *)&fw);
            if (ret <= 0) 
                return ret;
            
            ota_progress_indicate(&phase, 100);
            goto _end;
        }

        ota_progress_indicate(&phase, (ofs * 100) / (fw.fh_isize << 1) + 50);
    }

_end:
    return 0;
}

static int bootloader_init(void) {
    const char *disk = NULL;
    int err;

    fw_curinfo.offset = FW_INFO_OFFSET;
    err = disk_device_open(FW_LOAD_DEVICE, &fw_curinfo.dd);
    if (err) {
        disk = FW_LOAD_DEVICE;
        err = -ENODEV;
        goto _err;
    }

    fw_loader.offset = FW_LOAD_OFFSET;
    err = disk_device_open(FW_LOAD_DEVICE, &fw_loader.dd);
    if (err) {
        disk = FW_LOAD_DEVICE;
        err = -ENODEV;
        goto _err;
    }
    
    fw_runner.offset = FW_RUN_OFFSET;
    err = disk_device_open(FW_RUN_DEVICE, &fw_runner.dd);
    if (err) {
        disk = FW_RUN_DEVICE;
        err = -ENODEV;
        goto _err;
    }
    pr_dbg("bootloader loadaddr(0x%x) execaddr(0x%x)\n", 
        fw_loader.offset, fw_runner.offset);
    pr_dbg("bootloader initialize completed!\n");
    return 0;
_err:
    pr_emerg("Not found disk(%s) and error code is %d)\n", disk, err);
    return err;
}

/*
 * Bootloader entry
 */
int general_boot(
    void (*boot)(void), 
    void (*notify)(const char *, int)) {
    // bool force_recovery = false;
    int trycnt = 3;
    int i = 0, err;
    struct nvram_desc *nvram;

    pr_info(
        "\n=====================================\n"
          "          General BootLoader         \n"
          "=====================================\n"
    );
    if (!boot) {
        pr_err("Invalid boot function\n");
        return -EINVAL;
    }

    nvram = sys_nvram_get();
    pr_dbg("Boot nvram address: %p\n", nvram);

    err = env_load_ram(nvram->env_ram, sizeof(nvram->env_ram));
    if (err)
        pr_err("load environment variable failed\n");
    err = bootloader_init();
    assert(err == 0);

    /*
     * If the new firmware is ready and we will update it.
     */
    if (is_fw_need_update()) {
        pr_info("firmware update beginning...\n");
_repeat_update:
        err = fw_copy_decomp(&fw_runner, &fw_loader, notify, "update");
        if (!err) {
            env_unset("crash-cnt");
            pr_info("firmware update success!\n");
            goto _do_boot;
        }
        if (i++ < trycnt)
            goto _repeat_update;

        pr_err("firmware update failed(%d)\n", err);
        goto _do_boot;
    }

    if (is_fw_need_recovery())
        env_set("ota-update", "yes", 1);
#if 0
    /*
     * If the current firmware has some problems and infinite reboot 
     */
    if (force_recovery || is_fw_need_recovery()) {
        if (!is_fw_can_recovery()) {
            pr_warn("firmware backup partition is invalid!!\n");
            goto _do_boot;
        }
        pr_info("firmware recovery beginning...\n");

_repeat_recovery:
        err = fw_copy_decomp(&fw_runner, &fw_recovery, notify, "recovery");
        if (!err) {
            pr_info("firmware recovery success!\n");
            goto _do_boot;
        }
        if (i++ < trycnt)
            goto _repeat_recovery;
        goto _err;
    }
#endif

_do_boot:
    pr_dbg("Starting firmware ...\n");
    if (is_fw_runtime_okay()) 
        env_unset("crash-cnt");
    env_set("crash", "yes", 1);
    env_unset("runtime");
    env_flush_ram(nvram->env_ram, sizeof(nvram->env_ram));

    /* Boot firmware and never return */
    boot();

    /* Should never reached here */
    return err;
}
