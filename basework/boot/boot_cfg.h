/*
 * Copyright 2022 wtcat
 */
#ifndef BASEWORK_BOOT_BOOT_CFG_H_
#define BASEWORK_BOOT_BOOT_CFG_H_

#define LOAD_DEVICE_BASE 0x01b00000
#define RUN_DEVICE_BASE  0x24000

#define FW_LOAD_DEVICE   "spi_flash"
#define FW_INFO_OFFSET   (LOAD_DEVICE_BASE + 0x0)
#define FW_LOAD_OFFSET   (LOAD_DEVICE_BASE + 0x1000)

#define FW_RUN_DEVICE   "spi_flash"
#define FW_RUN_OFFSET  RUN_DEVICE_BASE

#ifndef FW_FLASH_BLKSIZE
#define FW_FLASH_BLKSIZE 4096
#endif

#endif /* BASEWORK_BOOT_BOOT_CFG_H_ */