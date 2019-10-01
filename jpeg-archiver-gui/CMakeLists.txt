project(jpeg-archiver-gui)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(CMAKE_VERSION VERSION_LESS "3.7.0")
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()

find_package(Qt5 COMPONENTS Widgets REQUIRED)

add_executable(jpeg-archiver
    main.cpp
    maindialog.cpp
    maindialog.ui
    config.cpp
    task.cpp
    processcontrollerthread.cpp
    jpeg-archiver.qrc
    processlogmodel.cpp
)

target_link_libraries(${CMAKE_PROJECT_NAME} Qt5::Widgets jpeg-recompress)
