zephyr_include_directories(.)

target_sources(basework
    PRIVATE
    os_platform.cc
    os_vflash_impl.cc
    os_timer_impl.c
    os_vdisk_impl.cc
)