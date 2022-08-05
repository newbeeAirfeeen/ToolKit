cmake_minimum_required(VERSION 3.10)
project(ToolKit)


set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED true)
add_definitions(-DASIO_STANDALONE -DASIO_HEADER_ONLY -DSRT_CORE_INTERNAL_DEBUG_LOG -DFMT_HEADER_ONLY -DOPENSSL_EXTRA)


if(WIN32)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_WIN32_WINNT=0x0601)
    set(CMAKE_C_FLAGS_DEBUG  /MTd)
    set(CMAKE_CXX_FLAGS_DEBUG /MTd)
endif()



add_subdirectory(3rdlib/wolfssl)
add_subdirectory(3rdlib/googletest)

add_definitions(-DSSL_ENABLE -DWOLFSSL_ENABLE)

include_directories(${CMAKE_SOURCE_DIR}/3rdlib/wolfssl)


#添加包含路径
aux_source_directory(${CMAKE_SOURCE_DIR}/src/Util SRC_LIST)
aux_source_directory(${CMAKE_SOURCE_DIR}/src/spdlog SRC_LIST)
aux_source_directory(${CMAKE_SOURCE_DIR}/src/net SRC_LIST)
aux_source_directory(${CMAKE_SOURCE_DIR}/src/net/ssl SRC_LIST)
aux_source_directory(${CMAKE_SOURCE_DIR}/src/protocol/stun SRC_LIST)
aux_source_directory(${CMAKE_SOURCE_DIR}/src/protocol/srt SRC_LIST)

add_library(${PROJECT_NAME}_static STATIC ${SRC_LIST})


set(LINK_LIB_LIST ${PROJECT_NAME}_static wolfssl CACHE INTERNAL "link library list")


target_link_libraries(${PROJECT_NAME}_static INTERFACE ${LINK_LIB_LIST})
target_include_directories(${PROJECT_NAME}_static PUBLIC ${CMAKE_SOURCE_DIR}/src
                                                  PUBLIC ${CMAKE_SOURCE_DIR}/src/net)

add_subdirectory(example)
add_subdirectory(test)