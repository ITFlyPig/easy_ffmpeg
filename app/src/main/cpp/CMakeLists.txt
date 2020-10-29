# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")

#JNI 路径
set(JNI_PATH ${CMAKE_SOURCE_DIR})
# 动态库 路径
set(SHARED_PATH ${CMAKE_SOURCE_DIR}/../nativeLibs/armeabi-v7a)

#MESSAGE("so库的路劲" ${ffmpeg_lib_dir})

#======================定义文件位置===================
#jni cpp
FILE(GLOB JNI_CPP ${JNI_PATH}/*.cpp)
FILE(GLOB JNI_H ${JNI_PATH}/*.h)

#video
FILE(GLOB VIDEO_CPP ${JNI_PATH}/video/*.cpp)
FILE(GLOB VIDEO_H ${JNI_PATH}/video)

#image
FILE(GLOB IMAGE_CPP ${JNI_PATH}/image/*.cpp)
FILE(GLOB IMAGE_H ${JNI_PATH}/image)

#log
FILE(GLOB LOG_CPP ${JNI_PATH}/log/*.cpp)
FILE(GLOB LOG_H ${JNI_PATH}/log)

#utils
FILE(GLOB UTILS_CPP ${JNI_PATH}/utils/*.cpp)
FILE(GLOB UTILS_H ${JNI_PATH}/utils)

#opengl
FILE(GLOB GL_CPP ${JNI_PATH}/opengl/*.cpp)
FILE(GLOB GL_H ${JNI_PATH}/opengl)
#常量
FILE(GLOB CONSTANT_H ${JNI_PATH}/constant)
FILE(GLOB CONSTANT_CPP ${JNI_PATH}/constant/*.cpp)

#egl
FILE(GLOB EGL_H ${JNI_PATH}/egl)
FILE(GLOB EGL_CPP ${JNI_PATH}/egl/*.cpp)

#======================加载头文件===================
include_directories(${JNI_H})
#ffmpeg头文件
include_directories(${JNI_PATH}/include)
#video
include_directories(${VIDEO_H})
#log
include_directories(${LOG_H})
#image
include_directories(${IMAGE_H})
#utils
include_directories(${UTILS_H})
#opengl
include_directories(${GL_H})
#常量
include_directories(${CONSTANT_H})
#EGL_H
include_directories(${CONSTANT_H})



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
add_library(native-lib SHARED
        ${JNI_CPP}
        ${VIDEO_CPP}
        ${LOG_CPP}
        ${IMAGE_CPP}
        ${UTILS_CPP}
        ${GL_CPP}
        ${CONSTANT_CPP}
        ${EGL_CPP})

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

        -landroid
        OpenSLES
        -lEGL
        -lGLESv2
        #解析Java传递过来的Bitmap需要
        jnigraphics

        # Links the target library to the log library
        # included in the NDK.
        ${log-lib})