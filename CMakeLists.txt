cmake_minimum_required(VERSION 3.10)
project(ToolKit VERSION 1)

add_definitions(-DASIO_STANDALONE
                -DASIO_HEADER_ONLY
 #               -DSRT_CORE_INTERNAL_DEBUG_LOG
                -DFMT_HEADER_ONLY
                #-DASIO_ENABLE_HANDLER_TRACKING
                -DENABLE_LOG
                -DOPENSSL_EXTRA)


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

add_library(${PROJECT_NAME}_static STATIC ${SRC_LIST} src/Util/endian.cpp)
if(LINUX)
    target_compile_options(${PROJECT_NAME}_static PUBLIC -std=c++11)
endif()

set(LINK_LIB_LIST ${PROJECT_NAME}_static wolfssl CACHE INTERNAL "link library list")


target_link_libraries(${PROJECT_NAME}_static INTERFACE ${LINK_LIB_LIST})
target_include_directories(${PROJECT_NAME}_static PUBLIC ${CMAKE_SOURCE_DIR}/src
                                                  PUBLIC ${CMAKE_SOURCE_DIR}/src/net)
if(NOT ANDROID)
add_subdirectory(example)
add_subdirectory(test)
add_subdirectory(benchmark)
endif()
