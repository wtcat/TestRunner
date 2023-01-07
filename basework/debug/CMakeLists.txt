
zephyr_library_sources(
    mb_trace.c
)

zephyr_library_sources_ifdef(CONFIG_CORTEXM_DWT cortex/dwt.c)