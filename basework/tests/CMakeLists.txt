
if (WINDOWS)
    set(CURRENT_TARGET ${TargetName})
else ()
    set(CURRENT_TARGET basework)
endif()

if (NOT WINDOWS)
    target_sources(${CURRENT_TARGET}
        PRIVATE
        # ${CMAKE_CURRENT_SOURCE_DIR}/rq_test.cc
        # ${CMAKE_CURRENT_SOURCE_DIR}/ccinit_test.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/blkdev_test.cc
        # ${CMAKE_CURRENT_SOURCE_DIR}/partition_test.cc
        # ${CMAKE_CURRENT_SOURCE_DIR}/bdbuf_blkdev_test.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/kfifo_test.c

    )
endif()

target_sources(${CURRENT_TARGET}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/timer_test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/env_test.cc
)
