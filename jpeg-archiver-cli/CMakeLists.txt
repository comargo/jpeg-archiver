project(jpeg-archiver-cli)

add_executable(${PROJECT_NAME}
    main.c
)
set_property(TARGET jpeg-archiver-cli PROPERTY C_STANDARD 11)

target_link_libraries(${PROJECT_NAME} jpeg-recompress)
if(WIN32)
add_subdirectory(getopt)
target_link_libraries(${PROJECT_NAME} getopt)
endif()
