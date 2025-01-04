cmake_minimum_required(VERSION 3.5)
project(psdk C)

set(CMAKE_C_FLAGS "-pthread -std=gnu99")
# set(CMAKE_CXX_FLAGS "-std=c++11 -pthread")
set(CMAKE_EXE_LINKER_FLAGS "-pthread")
set(CMAKE_C_COMPILER "arm-linux-gnueabihf-gcc")
# set(CMAKE_CXX_COMPILER "g++")
add_definitions(-D_GNU_SOURCE)

if (NOT USE_SYSTEM_ARCH)
    add_definitions(-DSYSTEM_ARCH_LINUX)
endif ()

if (BUILD_TEST_CASES_ON MATCHES TRUE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs -ftest-coverage -lgcov")
endif ()

set(PACKAGE_NAME payloadsdk)

## "uname -m" to auto distinguish Manifold2-G or Manifold2-C
# execute_process(COMMAND uname -m
#         OUTPUT_VARIABLE DEVICE_SYSTEM_ID)

# if (DEVICE_SYSTEM_ID MATCHES x86_64)
#     set(TOOLCHAIN_NAME x86_64-linux-gnu-gcc)
#     add_definitions(-DPLATFORM_ARCH_x86_64=1)
# elseif (DEVICE_SYSTEM_ID MATCHES aarch64)
    set(TOOLCHAIN_NAME arm-linux-gnueabihf-gcc)
    add_definitions(-DPLATFORM_ARCH_arm=1)
    # add_definitions(-DPLATFORM_ARCH_aarch64=1)
# else ()
#     message(FATAL_ERROR "FATAL: Please confirm your platform.")
# endif ()

file(GLOB_RECURSE MODULE_COMMON_SRC ../common/*.c)
file(GLOB_RECURSE MODULE_HAL_SRC hal/*.c)
file(GLOB_RECURSE MODULE_APP_SRC application/*.c)
file(GLOB_RECURSE DJI_MODULE_SRC ../../dji_module/*.c)
file(GLOB_RECURSE CZI_PAYLOAD_SRC ../../czi_payload/*.c)

include_directories(../../dji_module)
include_directories(../../czi_payload)
include_directories(../common)
include_directories(../manifold2/application)

include_directories (../../czi_payload/czi_log)
include_directories (../../czi_payload/czi_elog)
include_directories (../../czi_payload/czi_elog/inc)
include_directories (../../czi_payload/czi_elog/plugins/file)
include_directories (../../czi_payload/czi_elog/src)
include_directories (../../czi_payload/czi_config)
include_directories (../../czi_payload/czi_transmission/mailbox)
include_directories (/usr/include/)
include_directories (../../czi_payload/czi_video)
include_directories (../../czi_payload/czi_video/ffmpeg-6.1.1)

aux_source_directory (../../czi_payload/czi_log CZI_LOG_LIST_SOURCE)
aux_source_directory (../../czi_payload/czi_elog CZI_ELOG_LIST_SOURCE)
aux_source_directory (../../czi_payload/czi_elog/inc INC_LIST_SOURCE)
aux_source_directory (../../czi_payload/czi_elog/plugins/file FILE_LIST_SOURCE)
aux_source_directory (../../czi_payload/czi_elog/src SRC_LIST_SOURCE)
aux_source_directory (../../czi_payload/czi_config CONFIG_LIST_SOURCE)
aux_source_directory (../../czi_payload/czi_transmission/mailbox MAILBOX_LIST_SOURCE)
aux_source_directory (../../czi_payload/czi_video CZI_VIDEO_LIST_SOURCE)



include_directories(../../../psdk_lib/include)
link_directories(../../../psdk_lib/lib/${TOOLCHAIN_NAME})
# link_libraries(${CMAKE_CURRENT_LIST_DIR}/../../../psdk_lib/lib/${TOOLCHAIN_NAME}/lib${PACKAGE_NAME}.a)
link_directories(../../../czi_lib/lib)

# include_directories(/opt/host/include)
# link_directories(/opt/host/lib)

if (OPUS) 
    add_definitions(-DCZI_OPUS)
    message("-- OPUS Enable") 
endif()

if (NOT EXECUTABLE_OUTPUT_PATH)
    set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
endif ()

add_executable(${PROJECT_NAME}
        ${MODULE_APP_SRC}
        ${DJI_MODULE_SRC}
        ${MODULE_COMMON_SRC}
        ${MODULE_HAL_SRC}
        ${CZI_PAYLOAD_SRC}
        ${CZI_LOG_LIST_SOURCE}
        ${CZI_ELOG_LIST_SOURCE}
        ${INC_LIST_SOURCE}
        ${FILE_LIST_SOURCE}
        ${SRC_LIST_SOURCE}
        ${CONFIG_LIST_SOURCE}
        ${MAILBOX_LIST_SOURCE}
        ${CZI_VIDEO_LIST_SOURCE})

# set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../common/3rdparty)
# find_package(OPUS REQUIRED)
# if (OPUS_FOUND)
#     message(STATUS "Found OPUS installed in the system")
#     message(STATUS " - Includes: ${OPUS_INCLUDE_DIR}")
#     message(STATUS " - Libraries: ${OPUS_LIBRARY}")

#     add_definitions(-DOPUS_INSTALLED)
#     target_link_libraries(${PROJECT_NAME} libopus.so)
# else ()
#     message(STATUS "Cannot Find OPUS")
# endif (OPUS_FOUND)

# find_package(LIBUSB REQUIRED)
# if (LIBUSB_FOUND)
#     message(STATUS "Found LIBUSB installed in the system")
#     message(STATUS " - Includes: ${LIBUSB_INCLUDE_DIR}")
#     message(STATUS " - Libraries: ${LIBUSB_LIBRARY}")

#     add_definitions(-DLIBUSB_INSTALLED)
#     target_link_libraries(${PROJECT_NAME} usb-1.0)
# else ()
#     message(STATUS "Cannot Find LIBUSB")
# endif (LIBUSB_FOUND)

target_link_libraries(${PROJECT_NAME} m)
target_link_libraries(${PROJECT_NAME} protobuf-c)

# 添加目标链接库
target_link_libraries(${PROJECT_NAME} rt)
target_link_libraries(${PROJECT_NAME} -lpthread)
# target_link_libraries(${PROJECT_NAME} /opt/host/aarch64-buildroot-linux-gnu/sysroot/lib/lib${PACKAGE_NAME}.a)
target_link_libraries(${PROJECT_NAME}  ${CMAKE_CURRENT_LIST_DIR}/../../../psdk_lib/lib/${TOOLCHAIN_NAME}/lib${PACKAGE_NAME}.a)

add_custom_command(TARGET ${PROJECT_NAME}
        PRE_LINK COMMAND cmake ..
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR})


