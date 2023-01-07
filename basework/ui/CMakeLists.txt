zephyr_library()

zephyr_library_sources(
    resource/rc_access.c
    resource/rc_actions_impl.c
)
zephyr_library_link_libraries(lvgl)