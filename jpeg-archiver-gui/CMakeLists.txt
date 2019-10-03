project(jpeg-archiver-gui)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(CMAKE_VERSION VERSION_LESS "3.7.0")
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()

find_package(Qt5 COMPONENTS Widgets REQUIRED)
find_package (Threads)

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
set_property(TARGET jpeg-archiver PROPERTY CXX_STANDARD 11)
target_link_libraries(jpeg-archiver Qt5::Widgets jpeg-recompress ${CMAKE_THREAD_LIBS_INIT})
