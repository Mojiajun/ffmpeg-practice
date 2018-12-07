cmake_minimum_required(VERSION 3.12)
project(h264_to_yuv C)

set(CMAKE_C_STANDARD 99)

set(INC_DIR /usr/local/ffmpeg/include)
set(LINK_DIR /usr/local/ffmpeg/lib)

include_directories(${INC_DIR})
link_directories(${LINK_DIR})

SET(CMAKE_BUILD_TYPE "Debug")

add_executable(h264_to_yuv main.c)
target_link_libraries(h264_to_yuv avcodec)
