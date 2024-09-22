cmake_minimum_required(VERSION 3.16)
project(jpeg-archiver-gui)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Widgets Concurrent REQUIRED)
qt_standard_project_setup()


qt_add_executable(jpeg-archiver WIN32
    main.cpp
    maindialog.h
    maindialog.cpp
    maindialog.ui
    config.h
    config.cpp
    jpeg-archiver.qrc
    processlogmodel.h
    processlogmodel.cpp
)
set_target_properties(jpeg-archiver PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON)
target_link_libraries(jpeg-archiver PRIVATE Qt6::Widgets Qt6::Concurrent jpeg-recompress)

install(TARGETS jpeg-archiver)
