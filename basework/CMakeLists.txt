zephyr_library_named(basework)

zephyr_include_directories(
    cpu/cortexm
    ${CMAKE_CURRENT_SOURCE_DIR}/../
	common_include/
)

zephyr_library_compile_options(
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:ARMClang>>:-Wextra>
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:ARMClang>>:-Wextra>
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:ARMClang>>:-Wno-unused-parameter>
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:ARMClang>>:-Wno-implicit-fallthrough>
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:ARMClang>>:-Wno-missing-field-initializers>
#    $<$<CXX_COMPILER_ID:GNU>:-Wno-cast-function-type>
)

zephyr_library_sources(
    rq.c
    env.c
)
if (CONFIG_CPLUSPLUS OR WINDOWS OR LINUX)
zephyr_library_sources(ccinit.cc)
endif()


add_subdirectory(lib)
add_subdirectory(os)
add_subdirectory(dev)
if (NOT WINDOWS)
	add_subdirectory(drive)
endif()
add_subdirectory_ifdef(CONFIG_GTEST googletest)
add_subdirectory_ifdef(CONFIG_BASEWORK_TEST tests)

if (NOT WINDOWS AND NOT LINUX)
    add_subdirectory(debug)
    add_subdirectory(ui)
endif()
