cmake_minimum_required(VERSION 3.16)
project(ToolKit VERSION 1)

if(${CMAKE_BUILD_TYPE} STREQUAL "")
    set(CMAKE_BUILD_TYPE Debug)
endif()


set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED true)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

option(ENABLE_SHARED "enable shared library" OFF)
option(ENABLE_TEST "enable unit test" ON)
option(ENABLE_WOLFSSL "enable wolfssl" OFF)
option(ENABLE_LOG "enable internal log" ON)



option(ENABLE_SRT "enable srt" ON)
option(ENABLE_STUN "enable stun" OFF)
option(ENABLE_URI "enable uri" ON)
option(ENABLE_EXAMPLE "enable example" ON)
option(ENABLE_APP "enable application" ON)

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
aux_source_directory(${CMAKE_SOURCE_DIR}/src/net SRC_LIST)
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


if(ENABLE_URI)
    aux_source_directory(${CMAKE_SOURCE_DIR}/src/protocol/uri SRC_LIST)
endif()


find_package(GooglePerftools)
if(TCMALLOC_FOUND AND GOOGLEPERFTOOLS_FOUND)
    message("-- enable google perf tools, include directory:${GooglePerftools_INCLUDE_DIR},library:${GooglePerftools_LIBRARIES},debug library:${GooglePerftools_LIBRARIES_DEBUG}")
    include_directories(${GooglePerftools_INCLUDE_DIR})
    if(${CMAKE_BUILD_TYPE} STREQUAL "Debug" )
        add_definitions(-DENABLE_PREF_TOOL)
        list(APPEND LINK_LIB_LIST ${GooglePerftools_LIBRARIES_DEBUG})
    else()
        list(APPEND LINK_LIB_LIST ${GooglePerftools_LIBRARIES})
    endif()
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
include_directories(${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/3rdlib/asio)

add_library(${TOOLKIT_TARGET_NAME} ${IS_STATIC} ${SRC_LIST} src/Util/endian.cpp)
target_link_libraries(${TOOLKIT_TARGET_NAME} PUBLIC ${LINK_LIB_LIST})
target_include_directories(${TOOLKIT_TARGET_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/net)


if(NOT ANDROID)

    if(ENABLE_EXAMPLE)
        add_subdirectory(example)
    endif()

    if(ENABLE_TEST)
        message("-- enable unittest")
        add_subdirectory(test)
    endif()

    if(ENABLE_APP)
        message("-- enable app")
        add_subdirectory(app)
    endif()

endif()
