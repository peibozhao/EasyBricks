
cmake_minimum_required(VERSION 3.11.0)
project(easy_bricks)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_STANDARD 17)

# message($<$<BOOL:${CMAKE_TOOLCHAIN_FILE}>:-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}>)

include(FetchContent)
include(ExternalProject)
# glog
FetchContent_Declare(log
    URL https://github.com/google/glog/archive/refs/tags/v0.4.0.tar.gz)
set(WITH_GFLAGS OFF CACHE BOOL "Glog use gflags or not")
set(BUILD_TESTING OFF CACHE BOOL "Build unitest or not")
FetchContent_MakeAvailable(log)
target_compile_options(glog PRIVATE -w)
include_directories(${log_BINARY_DIR})
# yaml
FetchContent_Declare(yaml
    URL https://github.com/jbeder/yaml-cpp/archive/refs/tags/yaml-cpp-0.6.3.tar.gz)
set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "Yaml build test or not")
FetchContent_MakeAvailable(yaml)
target_compile_options(yaml-cpp PRIVATE -w)
include_directories(${yaml_SOURCE_DIR})
# json
FetchContent_Declare(json
    URL https://github.com/nlohmann/json/archive/refs/tags/v3.9.1.tar.gz)
set(JSON_BuildTests CACHE STRING OFF)
FetchContent_MakeAvailable(json)
include_directories(${json_SOURCE_DIR}/include)
# http
FetchContent_Declare(http
    URL https://github.com/yhirose/cpp-httplib/archive/refs/tags/v0.8.8.tar.gz)
FetchContent_MakeAvailable(http)
include_directories(${http_SOURCE_DIR})
# jpeg-turbo
ExternalProject_Add(jpeg-turbo
    URL https://github.com/libjpeg-turbo/libjpeg-turbo/archive/refs/tags/2.1.4.tar.gz
    PREFIX depends
    CMAKE_ARGS
        $<$<BOOL:${CMAKE_TOOLCHAIN_FILE}>:-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}>
        -DENABLE_SHARED=OFF
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
    INSTALL_COMMAND "")
ExternalProject_Get_Property(jpeg-turbo SOURCE_DIR BINARY_DIR)
include_directories(${SOURCE_DIR})
link_directories(${BINARY_DIR})

include_directories(source)
set(EASYBRICKS_SOURCE_LIST
    source/ocr/paddle_ocr.cpp
    source/player/common_player.cpp
    source/player/blhx_player.cpp
    source/facade/game_system.cpp
    source/utils/util_functions.cpp)

# JNI in android
if (${CMAKE_SYSTEM_NAME} STREQUAL Android)
    set(EASYBRICKS_SOURCE_LIST ${EASYBRICKS_SOURCE_LIST} ${CMAKE_SOURCE_DIR}/example/android/jni/EasyBricks_EasyBricks.cpp)
endif ()

add_library(easy_bricks SHARED ${EASYBRICKS_SOURCE_LIST})
target_link_libraries(easy_bricks yaml-cpp glog nlohmann_json turbojpeg)
add_dependencies(easy_bricks jpeg-turbo)

# JNI in android
if (${CMAKE_SYSTEM_NAME} STREQUAL Android)
    add_custom_target(jni_head
        COMMAND javac -h . EasyBricks.java PlayOperation.java
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/example/android/jni)
    # add_custom_command(OUTPUT EasyBricks_EasyBricks.h
    #     COMMAND javac ARGS -h . EasyBricks.java PlayOperation.java
    #     WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/example/android/jni)
    add_dependencies(easy_bricks jni_head)
    find_library(ANDROID_LOG_LIB log)
    target_link_libraries(easy_bricks ${ANDROID_LOG_LIB})
endif ()

# Demo in linux
if (${CMAKE_SYSTEM_NAME} STREQUAL Linux)
    include_directories(opencv4)
    set(DEMO_SOURCE_LIST
        example/linux/x11_screencap.cpp
        example/linux/x11_event.cpp
        example/linux/main.cpp)
    add_executable(demo ${DEMO_SOURCE_LIST})
    add_dependencies(demo easy_bricks)
    target_link_libraries(demo X11 easy_bricks)
endif ()

