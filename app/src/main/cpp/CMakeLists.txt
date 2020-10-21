# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

#JNI 路径
set(JNI_PATH ${CMAKE_SOURCE_DIR})
# 动态库 路径
set(SHARED_PATH ${CMAKE_SOURCE_DIR}/../nativeLibs/armeabi-v7a)

#MESSAGE("so库的路劲" ${ffmpeg_lib_dir})

#======================定义文件位置===================
#jni cpp
FILE(GLOB JNI_CPP ${JNI_PATH}/*.cpp)
FILE(GLOB JNI_H ${JNI_PATH}/*.h)


#======================加载头文件===================
include_directories(${JNI_H})
#ffmpeg头文件
include_directories(${JNI_PATH}/include)

#======================加载库文件===================

#加载ffmpeg动态库
add_library(avcodec SHARED IMPORTED)
set_target_properties(avcodec PROPERTIES IMPORTED_LOCATION ${SHARED_PATH}/libavcodec.so)

add_library(avformat SHARED IMPORTED)
set_target_properties(avformat PROPERTIES IMPORTED_LOCATION ${SHARED_PATH}/libavformat.so)

add_library(avutil SHARED IMPORTED)
set_target_properties(avutil PROPERTIES IMPORTED_LOCATION ${SHARED_PATH}/libavutil.so)

add_library(swscale SHARED IMPORTED)
set_target_properties(swscale PROPERTIES IMPORTED_LOCATION ${SHARED_PATH}/libswscale.so)

add_library(swresample SHARED IMPORTED)
set_target_properties(swresample PROPERTIES IMPORTED_LOCATION ${SHARED_PATH}/libswresample.so)

add_library(avfilter SHARED IMPORTED)
set_target_properties(avfilter PROPERTIES IMPORTED_LOCATION ${SHARED_PATH}/libavfilter.so)

#通过源码，添加自己编写的库
add_library(native-lib SHARED ${JNI_CPP})

#找到系统提供的库
find_library(log-lib log)

#链接库
target_link_libraries(
        native-lib
        avcodec
        avfilter
        avformat
        avutil
        swresample
        swscale

        # Links the target library to the log library
        # included in the NDK.
        ${log-lib})