

message("-- 构建类型 ${CMAKE_BUILD_TYPE} 模式")
message("-- ${PROJECT_NAME}的根目录: ${CMAKE_SOURCE_DIR}")
message("-- 系统信息：${CMAKE_SYSTEM}")
message("-- 构建平台：${CMAKE_SYSTEM_NAME}")
message("-- 处理器: ${CMAKE_SYSTEM_PROCESSOR}")
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    message("-- 处理器位数: 64位")
elseif()
    message("-- 处理器位数: 32位")
endif()
message("-- 编译器: ${CMAKE_CXX_COMPILER}")