
zephyr_library_sources(
    ${CMAKE_CURRENT_SOURCE_DIR}/iovprintf.c
    ${CMAKE_CURRENT_SOURCE_DIR}/printer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/timer/timer_list.c
    ${CMAKE_CURRENT_SOURCE_DIR}/env_core.c
    ${CMAKE_CURRENT_SOURCE_DIR}/disklog.c
)

if (NOT WINDOWS)
    zephyr_library_sources(
        ${CMAKE_CURRENT_SOURCE_DIR}/bitops.c
    )
endif()

zephyr_library_sources(
    ${CMAKE_CURRENT_SOURCE_DIR}/bdbuf.c
    ${CMAKE_CURRENT_SOURCE_DIR}/bdbuf_cfg.c
    ${CMAKE_CURRENT_SOURCE_DIR}/bdbuf_ioctl.c
    ${CMAKE_CURRENT_SOURCE_DIR}/diskdevs.c
)