#创建文件夹
#设置可执行文件，库的输出路径
function(set_target_location BIN_LOCATION LIB_LOCATION)

    set(LIBRARY_OUTPUT_PATH  ${BIN_LOCATION}  CACHE STRING "二进制目标文件的构建路径")
    set(EXECUTABLE_OUTPUT_PATH ${LIB_LOCATION} CACHE STRING "库的构建路径")

    file(MAKE_DIRECTORY ${LIBRARY_OUTPUT_PATH})
    file(MAKE_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})
    message("-- 二进制目标文件的构建路径: ${LIBRARY_OUTPUT_PATH}, 库的构建路径: ${EXECUTABLE_OUTPUT_PATH}")
endfunction()
