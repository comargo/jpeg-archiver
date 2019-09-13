cmake_minimum_required(VERSION 3.6)

project(jpeg-archiver)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(CMAKE_VERSION VERSION_LESS "3.7.0")
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()

find_package(Qt5 COMPONENTS Widgets REQUIRED)

add_executable(${CMAKE_PROJECT_NAME}
    main.cpp
    maindialog.cpp
    maindialog.ui
    config.cpp
    processcontrollerthread.cpp
    jpeg-archiver.qrc
)

target_link_libraries(${CMAKE_PROJECT_NAME} Qt5::Widgets)
