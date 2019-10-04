set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_INCLUDE_CURRENT_DIR_IN_INTERFACE ON)

add_subdirectory(iqa)
project(jpeg-recompress LANGUAGES C)
add_library(jpeg-recompress
    jpeg-recompress.c
    image_compare.c)

if(WIN32)
    # in case of Windows, it is possible that jpeg.dll is build with wrong version of CRT
    # it makes free() function to fall
    include(ExternalProject)
    ExternalProject_Add(libjpeg-turbo
        GIT_REPOSITORY https://github.com/libjpeg-turbo/libjpeg-turbo.git
        GIT_TAG 2.0.3
        CMAKE_ARGS -DENABLE_SHARED=FALSE
        CMAKE_CACHE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_CURRENT_BINARY_DIR}/libjpeg-turbo
        )
    target_include_directories(jpeg-recompress PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/libjpeg-turbo/include)
    target_link_libraries(jpeg-recompress PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/libjpeg-turbo/lib/jpeg-static.lib)
else()
    find_package(JPEG REQUIRED)
    target_link_libraries(jpeg-recompress PUBLIC JPEG::JPEG)
endif()

target_link_libraries(jpeg-recompress PRIVATE iqa)
