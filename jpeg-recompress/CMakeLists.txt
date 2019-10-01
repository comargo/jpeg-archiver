set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_INCLUDE_CURRENT_DIR_IN_INTERFACE ON)

add_subdirectory(iqa)
project(jpeg-recompress LANGUAGES C)
find_package(JPEG REQUIRED)
add_library(jpeg-recompress
    jpeg-recompress.c
    image_compare.c)
target_link_libraries(jpeg-recompress 
    PUBLIC JPEG::JPEG)
target_link_libraries(jpeg-recompress PRIVATE iqa)
