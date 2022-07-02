#version required
cmake_minimum_required(VERSION 2.8)

PROJECT(START_WEBRTC)

set(WEBRTC_CURRENT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(WEBRTC_BUILD_ROOT ${CMAKE_BINARY_DIR})


set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -std=c99 -DWEBRTC_POSIX")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -g -fno-rtti -fno-exceptions -std=c++11 -DWEBRTC_POSIX")

## api
file(GLOB_RECURSE API_SOURCE_FILES
        ${WEBRTC_CURRENT_DIR}/api/*.cc)
#message("=== api files=${API_SOURCE_FILES}")

## base
file(GLOB_RECURSE BASE_SOURCE_FILES
        ${WEBRTC_CURRENT_DIR}/rtc_base/*.cc
        ${WEBRTC_CURRENT_DIR}/common_types.cc
        ${WEBRTC_CURRENT_DIR}/config.cc)

if (${PLATFORM} STREQUAL ${MACOS})
    set(BASE_SOURCE_FILES ${BASE_SOURCE_FILES} ${WEBRTC_CURRENT_DIR}/base/macconversion.cc ${WEBRTC_CURRENT_DIR}/base/macifaddrs_converter.cc)
endif()
#message("=== base files=${BASE_SOURCE_FILES}")

## call
file(GLOB_RECURSE CALL_SOURCE_FILES
        ${WEBRTC_CURRENT_DIR}/call/call.cc)
#message("=== call files=${CALL_SOURCE_FILES}")

## media
file(GLOB_RECURSE MEDIA_SOURCE_FILES
        ${WEBRTC_CURRENT_DIR}/media/*.cc)
#message("=== media source files=${MEDIA_SOURCE_FILES}")

## p2p
file(GLOB_RECURSE P2P_SOURCE_FILES
        ${WEBRTC_CURRENT_DIR}/p2p/*.cc)
#message("=== p2p source files=${P2P_SOURCE_FILES}")

## pc
file(GLOB_RECURSE PC_SOURCE_FILES
        ${WEBRTC_CURRENT_DIR}/pc/*.cc)
#message("=== pc source files=${P2P_SOURCE_FILES}")

## system_wrappers
file(GLOB_RECURSE SYSTEM_WRAPPERS_SOURCE_FILES
        ${WEBRTC_CURRENT_DIR}/system_wrappers/source/*.cc)

#message("=== system_wrappers files=${SYSTEM_WRAPPERS_SOURCE_FILES}")

## video
file(GLOB_RECURSE VIDEO_SOURCE_FILES
        ${WEBRTC_CURRENT_DIR}/video/*.cc)
#message("=== video files=${VIDEO_SOURCE_FILES}")

## modules/pacing
file(GLOB_RECURSE MODULES_PACING_SOURCE_FILES
        ${WEBRTC_CURRENT_DIR}/modules/pacing/paced_sender.cc
        ${WEBRTC_CURRENT_DIR}/modules/pacing/packet_router.cc)
#message("=== modules pacing files=${MODULES_PACING_SOURCE_FILES}")

## modules/rtp_rtcp
file(GLOB_RECURSE MODULES_RTP_RTCP_SOURCE_FILES
        ${WEBRTC_CURRENT_DIR}/modules/rtp_rtcp/source/*.cc)
#message("=== modules rtp_rtcp files=${MODULES_RTP_RTCP_SOURCE_FILES}")

#######################
set(WEBRTC_ALL_FILE ${API_SOURCE_FILES}
        ${BASE_SOURCE_FILES}
        ${CALL_SOURCE_FILES}
        ${MEDIA_SOURCE_FILES}
        ${P2P_SOURCE_FILES}
        ${PC_SOURCE_FILES}
        ${SYSTEM_WRAPPERS_SOURCE_FILES}
        ${VIDEO_SOURCE_FILES}
        ${MODULES_PACING_SOURCE_FILES}
        ${MODULES_RTP_RTCP_SOURCE_FILES})


####################### 以下是为了编译webrtc动态库需要额外编译的源文件。
set(WEBRTC_DYNAMIC_FILES
        ${MODULES_VIDEO_CODING_SOURCE_FILES}
        ${GLOB_RECURSE}
        ${MODULES_VIDEO_CAPTURE_SOURCE_FILES}
        ${WEBRTC_ALL_FILE})

include_directories(${WEBRTC_CURRENT_DIR}
        ${WEBRTC_CURRENT_DIR}/third_party/abseil-cpp)

add_library(webrtc_static STATIC ${WEBRTC_ALL_FILE})
set_target_properties(webrtc_static PROPERTIES OUTPUT_NAME "webrtc_static")
set_target_properties(webrtc_static PROPERTIES CLEAN_DIRECT_OUTPUT 1)