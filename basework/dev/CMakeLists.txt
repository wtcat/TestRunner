
zephyr_library_sources(
    ${CMAKE_CURRENT_SOURCE_DIR}/disk.c
    ${CMAKE_CURRENT_SOURCE_DIR}/partition.c
    ${CMAKE_CURRENT_SOURCE_DIR}/partition_file.c
    ${CMAKE_CURRENT_SOURCE_DIR}/partitions_cfg.c
    ${CMAKE_CURRENT_SOURCE_DIR}/paritions_usr_cfg.c
    ${CMAKE_CURRENT_SOURCE_DIR}/clock_timer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/clock_base.c
)
zephyr_library_sources_ifndef(CONFIG_BOOTLOADER
    ${CMAKE_CURRENT_SOURCE_DIR}/blkdev.c
)

