find_package(SDL2_image)
find_package(SDL2)
find_package(FFmpeg)
if(SDL2_FOUND AND SDL2_IMAGE_FOUND)
    message("-- found sdl2 library, sdl2_image library:${SDL2_IMAGE_INCLUDE_DIRS};${SDL2_INCLUDE_DIR};${SDL2_IMAGE_INCLUDE_DIRS};
    ${SDL2_IMAGE_LIBRARIES};${SDL2_INCLUDE_DIR};${SDL2_LIBRARY}")
    include_directories(${SDL2_IMAGE_INCLUDE_DIRS} ${SDL2_INCLUDE_DIR})
    add_subdirectory(srtplayer)
endif()