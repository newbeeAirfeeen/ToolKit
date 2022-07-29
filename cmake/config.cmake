find_package(Git REQUIRED)
execute_process(
        COMMAND ${GIT_EXECUTABLE} log -1 --pretty=format:%H
        OUTPUT_VARIABLE GIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
set(TCPBUFFER_VERSION "v2.1.18")
set(TCPBUFFER_GIT_HASH ${GIT_HASH})
configure_file(${CMAKE_SOURCE_DIR}/src/tcpbuffer_config.h.in ${CMAKE_SOURCE_DIR}/src/tcpbuffer/tcpbuffer/tcpbuffer_config.h)
