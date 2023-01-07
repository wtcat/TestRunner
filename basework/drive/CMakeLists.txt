if (NOT LINUX AND NOT WINDOWS)
zephyr_library()

zephyr_library_compile_options(
	-Wno-enum-conversion
)

zephyr_library_sources(
	motor/motor.c
)
endif()