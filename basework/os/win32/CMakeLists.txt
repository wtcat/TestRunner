zephyr_include_directories(.)

target_sources(basework
    PRIVATE
    os_platform.cc
    os_fs_impl.c
    os_timer_impl.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../lib/timer/timer_list.c
)