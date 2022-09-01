cmake_minimum_required(VERSION 3.10)
project(ToolKit VERSION 1)


option(ENABLE_SHARED "是否开启动态库编译" OFF)


if(ENABLE_SHARED)
    set(TOOLKIT_TARGET_NAME ${PROJECT_NAME}_shared CACHE INTERNAL "project name")
    set(BUILD_SHARED_ALL ON CACHE INTERNAL "build shared all")
else()
    set(TOOLKIT_TARGET_NAME ${PROJECT_NAME}_static wolfssl CACHE INTERNAL "project name")
endif()


add_definitions(-DASIO_STANDALONE
                -DASIO_HEADER_ONLY
                -DFMT_HEADER_ONLY
                -DENABLE_LOG
                -DOPENSSL_EXTRA)
        #-DSRT_CORE_INTERNAL_DEBUG_LOG
        #-DASIO_ENABLE_HANDLER_TRACKING)


if(WIN32)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_WIN32_WINNT=0x0601)
    set(CMAKE_C_FLAGS_DEBUG  /MTd)
    set(CMAKE_C_FLAGS_RELEASE /O2)
    set(CMAKE_CXX_FLAGS_DEBUG /MTd)
    set(CMAKE_CXX_FLAGS_RELEASE /O2)
elseif(LINUX)
    add_compile_options(-Wreturn-local-addr)
endif()


add_subdirectory(3rdlib/wolfssl)
add_subdirectory(3rdlib/googletest)


set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED true)

add_definitions(-DSSL_ENABLE -DWOLFSSL_ENABLE)

include_directories(${CMAKE_SOURCE_DIR}/3rdlib/wolfssl)


#添加包含路径
aux_source_directory(${CMAKE_SOURCE_DIR}/src/Util SRC_LIST)
aux_source_directory(${CMAKE_SOURCE_DIR}/src/spdlog SRC_LIST)
aux_source_directory(${CMAKE_SOURCE_DIR}/src/net SRC_LIST)
aux_source_directory(${CMAKE_SOURCE_DIR}/src/net/ssl SRC_LIST)
aux_source_directory(${CMAKE_SOURCE_DIR}/src/protocol/stun SRC_LIST)
aux_source_directory(${CMAKE_SOURCE_DIR}/src/protocol/srt SRC_LIST)


if(ENABLE_SHARED)
    add_library(${TOOLKIT_TARGET_NAME} SHARED ${SRC_LIST} src/Util/endian.cpp)
    target_link_libraries(${TOOLKIT_TARGET_NAME} PUBLIC wolfssl)
    set(LINK_LIB_LIST ${TOOLKIT_TARGET_NAME} CACHE INTERNAL "link library list")
else()
    add_library(${TOOLKIT_TARGET_NAME} STATIC ${SRC_LIST} src/Util/endian.cpp)
    set(LINK_LIB_LIST ${TOOLKIT_TARGET_NAME} wolfssl CACHE INTERNAL "link library list")
endif()


if(LINUX)
    target_compile_options(${TOOLKIT_TARGET_NAME} PUBLIC -std=c++11)
endif()

target_include_directories(${TOOLKIT_TARGET_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/src
                                                  PUBLIC ${CMAKE_SOURCE_DIR}/src/net)
if(NOT ANDROID)
add_subdirectory(example)
add_subdirectory(test)
add_subdirectory(benchmark)
endif()
