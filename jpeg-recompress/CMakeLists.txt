set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_INCLUDE_CURRENT_DIR_IN_INTERFACE ON)

add_subdirectory(iqa)
project(jpeg-recompress LANGUAGES C)
add_library(jpeg-recompress
    jpeg-recompress.h
    jpeg-recompress.cpp
    image_compare.h
    image_compare.cpp
    jpeglib_cpp.h)
if(WIN32)
    include(FindJpegTurbo.cmake)
else()
find_package(JPEG REQUIRED)
endif()
target_link_libraries(jpeg-recompress PUBLIC JPEG::JPEG)

target_link_libraries(jpeg-recompress PRIVATE iqa)
