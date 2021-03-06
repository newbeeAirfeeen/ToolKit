cmake_minimum_required(VERSION 3.10)
project(ToolKit)


set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED true)
add_compile_options(-DASIO_STANDALONE -DASIO_HEADER_ONLY)

set(CMAKE_PREFIX_PATH /usr/local/openssl)
set(LINK_LIB_LIST "")
find_package(OpenSSL)

if(OPENSSL_FOUND)
    message("-- 开启OpenSSL, path:${OPENSSL_INCLUDE_DIR}, 将要链接的OpenSSL库: ${OPENSSL_LIBRARIES}")
    include_directories(${OPENSSL_INCLUDE_DIR})
    list(APPEND LINK_LIB_LIST ${OPENSSL_LIBRARIES})
    add_definitions(-DSSL_ENABLE)
endif()


if(WIN32)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

#添加包含路径
aux_source_directory(${CMAKE_SOURCE_DIR}/src/Util SRC_LIST)
aux_source_directory(${CMAKE_SOURCE_DIR}/src/spdlog SRC_LIST)
aux_source_directory(${CMAKE_SOURCE_DIR}/src/net SRC_LIST)
aux_source_directory(${CMAKE_SOURCE_DIR}/src/net/ssl SRC_LIST)
add_library(ToolKit STATIC ${SRC_LIST})
target_link_libraries(ToolKit INTERFACE ${LINK_LIB_LIST})
target_include_directories(ToolKit PUBLIC ${CMAKE_SOURCE_DIR}/src
                                   PUBLIC ${CMAKE_SOURCE_DIR}/src/net)
add_subdirectory(test)