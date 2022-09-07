cmake_minimum_required(VERSION 3.10)
project(ToolKit VERSION 1)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED true)

option(ENABLE_SHARED "enable shared library" OFF)
option(ENABLE_TEST "enable unit test" OFF)
option(ENABLE_WOLFSSL "enable wolfssl" OFF)
option(ENABLE_LOG "enable internal log" ON)



option(ENABLE_SRT "enable srt" ON)
option(ENABLE_STUN "enable stun" OFF)
option(ENABLE_EXAMPLE "enable example" ON)


add_definitions(-DASIO_STANDALONE
                -DASIO_HEADER_ONLY
                -DFMT_HEADER_ONLY)
        #-DASIO_ENABLE_HANDLER_TRACKING)


if(WIN32)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_WIN32_WINNT=0x0601)
    set(CMAKE_C_FLAGS_DEBUG  /MTd)
    set(CMAKE_C_FLAGS_RELEASE /O2)
    set(CMAKE_CXX_FLAGS_DEBUG /MTd)
    set(CMAKE_CXX_FLAGS_RELEASE /O2)
endif()

# 源代码
set(SRC_LIST "")
# 待链接的库
set(LINK_LIB_LIST "")

#添加包含路径
aux_source_directory(${CMAKE_SOURCE_DIR}/src/spdlog SRC_LIST)

if(ENABLE_TEST)
    add_subdirectory(3rdlib/googletest)
endif()


if(ENABLE_WOLFSSL)
    add_definitions(-DWOLFSSL_ENABLE -DOPENSSL_EXTRA)
    add_subdirectory(3rdlib/wolfssl)
    include_directories(${CMAKE_SOURCE_DIR}/3rdlib/wolfssl)
    aux_source_directory(${CMAKE_SOURCE_DIR}/src/net/ssl SRC_LIST)
    list(APPEND LINK_LIB_LIST wolfssl)
endif()

if(ENABLE_LOG)
    add_definitions(-DENABLE_LOG)
endif()


if(ENABLE_STUN)
    add_definitions(-DENABLE_STUN)
    aux_source_directory(${CMAKE_SOURCE_DIR}/src/protocol/stun SRC_LIST)
    list(APPEND SRC_LIST src/Util/crc32.cpp src/Util/MD5.cpp)
endif()

if(ENABLE_SRT)
    aux_source_directory(${CMAKE_SOURCE_DIR}/src/protocol/srt SRC_LIST)
endif()

# 静态或动态编译
set(IS_STATIC STATIC)
if(ENABLE_SHARED)
    set(IS_STATIC SHARED)
endif()


if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    list(APPEND LINK_LIB_LIST -lpthread)
endif()

string(TOLOWER ${IS_STATIC} shared_or_static)
message("-- library use ${IS_STATIC} build")
set(TOOLKIT_TARGET_NAME ${PROJECT_NAME}_${shared_or_static} CACHE INTERNAL "project name")
set(BUILD_SHARED_ALL ${IS_STATIC} CACHE INTERNAL "build ${IS_STATIC} all")
set(LINK_LIBS_GLOBAL ${TOOLKIT_TARGET_NAME} ${LINK_LIB_LIST} CACHE INTERNAL "build ${IS_STATIC} all")
include_directories(${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/net)

add_library(${TOOLKIT_TARGET_NAME} ${IS_STATIC} ${SRC_LIST} src/Util/endian.cpp)
target_link_libraries(${TOOLKIT_TARGET_NAME} PUBLIC ${LINK_LIB_LIST})
target_include_directories(${TOOLKIT_TARGET_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/net)


if(NOT ANDROID)

    if(ENABLE_EXAMPLE)
        add_subdirectory(example)
    endif()

    if(ENABLE_TEST)
        add_subdirectory(test)
    endif()

endif()
