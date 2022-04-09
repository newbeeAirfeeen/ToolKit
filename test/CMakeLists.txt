aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} TEST_EXE)

foreach(item ${TEST_EXE})
    string(REGEX MATCH ".*[/\\](.*).cpp$" item_src ${item})
    message("-- 正在构建 ${item} --> ${CMAKE_MATCH_1} ${CMAKE_EXECUTABLE_SUFFIX}")
    add_executable(${CMAKE_MATCH_1} ${item})
    target_link_libraries(${CMAKE_MATCH_1} PUBLIC ToolKit)
endforeach()