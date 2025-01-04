cmake_minimum_required(VERSION 3.22)

set(USE_SYSTEM_ARCH LINUX)

project(entry)

# Disable in-source builds to prevent source tree corruption.
if (" ${CMAKE_SOURCE_DIR}" STREQUAL " ${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR "FATAL: In-source builds are not allowed.
       You should create a separate directory for build files.")
endif ()


add_definitions(-DSYSTEM_ARCH_LINUX)
add_subdirectory(source/platform/manifold2)
# add_subdirectory(samples/sample_c++/platform/linux/manifold2)

set(LIBRARY_PATH psdk_lib/lib/arm-linux-gnueabihf-gcc)

install(FILES ${LIBRARY_PATH}/libpayloadsdk.a
        DESTINATION "${CMAKE_INSTALL_PREFIX}/lib"
        )

install(DIRECTORY psdk_lib/include
        DESTINATION "${CMAKE_INSTALL_PREFIX}"
        )

add_custom_target(${PROJECT_NAME} ALL)
