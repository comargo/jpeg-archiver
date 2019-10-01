cmake_minimum_required(VERSION 3.12)

project(jpeg-archiver)


#include(ExternalProject)
#ExternalProject_Add(jpeg-recompress SOURCE_DIR jpeg-recompress)

add_subdirectory(jpeg-recompress)
add_subdirectory(jpeg-archiver-gui)
add_subdirectory(jpeg-archiver-cli)

