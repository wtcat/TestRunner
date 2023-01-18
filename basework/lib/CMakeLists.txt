
zephyr_library_sources(
    ${CMAKE_CURRENT_SOURCE_DIR}/iovprintf.c
    ${CMAKE_CURRENT_SOURCE_DIR}/printer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/timer/timer_list.c
    ${CMAKE_CURRENT_SOURCE_DIR}/env_core.c
    ${CMAKE_CURRENT_SOURCE_DIR}/disklog.c
    ${CMAKE_CURRENT_SOURCE_DIR}/libitoa.c
    ${CMAKE_CURRENT_SOURCE_DIR}/libassert.c
    ${CMAKE_CURRENT_SOURCE_DIR}/libstrlcpy.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lz4.c
    ${CMAKE_CURRENT_SOURCE_DIR}/libcrc16.c
)

if (NOT WINDOWS)
    zephyr_library_sources(
        ${CMAKE_CURRENT_SOURCE_DIR}/bitops.c
    )
endif()

add_subdirectory(block)