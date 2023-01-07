/*
 * Copyright 2022 wtcat
 */
#ifndef BASEWORK_BOOT_H_
#define BASEWORK_BOOT_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C"{
#endif
#define FH_NAME_MAX 32

struct firmware_header {
    uint32_t fh_magic; /* Firmware header mangic number */
#define FH_MAGIC 0xa578875a
    uint32_t fh_version; /* Firmware version number */
    uint32_t fh_dcrc;  /* Firmware data crc checksum */
    uint32_t fh_size;  /* Firmware data size (not include header)*/
    uint32_t fh_isize; /* Firmware origin size (not include header)*/
    uint8_t  fh_comp; /* Compression type */
#define FH_COMP_NONE 0x00
#define FH_COMP_LZ4  0x01
    uint8_t  fh_arch; /* CPU architecture */
#define FH_ARCH_ARM  0x01
    uint8_t  fh_os;   /* Operation system */
#define FH_OS_ZEPHYR 0x01
    uint8_t  reserved[5];
    uint32_t fh_entry; /* Entry address */
    uint8_t  fh_name[FH_NAME_MAX]; /* Firmware name */
};

struct firmware_pointer {
    uint32_t fh_magic;
    uint32_t fh_dcrc;
};

_Static_assert(sizeof(struct firmware_header) == 64, "");

#define FW_BOOT_PARTITION "fw-boot"
#define FW_SWAP_PARTITION "fw-swap"
#define FW_SAVE_PARTITION "fw-save"


#define FW_LOAD_DEVICE  "spi-flash"
#define FW_LOAD_ADDRESS 0x0

#define FW_RUN_DEVICE   "spi_flash"
#define FW_RUN_ADDRESS  0x0

#ifdef __cplusplus
}
#endif
#endif /* BASEWORK_BOOT_H_ */
