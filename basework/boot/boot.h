/*
 * Copyright 2022 wtcat
 */
#ifndef BASEWORK_BOOT_H_
#define BASEWORK_BOOT_H_

#include "basework/generic.h"
#include "basework/boot/boot_cfg.h"

#ifdef __cplusplus
extern "C"{
#endif

#define FH_NAME_MAX 32
#define FH_MAGIC 0xa578875a

#define FIRMWARE_BASE \
    uint32_t fh_magic; /* Firmware header mangic number */ \
    uint32_t fh_devid; /* Firmware Device ID */ \
    uint32_t fh_version; /* Firmware version number */ \
    uint32_t fh_dcrc; /* Firmware data crc checksum */ \



struct firmware_header {
    FIRMWARE_BASE
    uint32_t fh_size;  /* Firmware data size (not include header)*/
    uint32_t fh_isize; /* Firmware origin size (not include header)*/
    uint8_t  fh_comp;  /* Compression type */
#define FH_COMP_NONE 0x00
#define FH_COMP_LZ4  0x01
#define FH_COMP_FASTLZ 0x02

    // uint8_t  reserved[5];
    uint32_t fh_entry; /* Entry address */
    uint8_t  fh_name[FH_NAME_MAX]; /* Firmware name */
};

struct firmware_pointer {
    FIRMWARE_BASE
};

STATIC_ASSERT(sizeof(struct firmware_header) == 64, "xx");

/*
 * general_boot - Boot firmware
 *
 * @boot: boot firmware callback
 * @notify: the callback that the update progress of firmware
 */
int general_boot(
    void (*boot)(void), 
    void (*notify)(const char *, int)
);

#ifdef __cplusplus
}
#endif
#endif /* BASEWORK_BOOT_H_ */
