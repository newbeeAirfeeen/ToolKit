add_executable(srtplayer srtplayer.cpp)
target_link_libraries(srtplayer PUBLIC ${SDL2_IMAGE_LIBRARIES} PUBLIC ${SDL2_LIBRARY})