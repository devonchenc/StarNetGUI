cmake_minimum_required(VERSION 3.21)

project(StarNetGUI LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
)

qt_add_executable(StarNetGUI WIN32
    main.cpp
    MainWindow.h
    MainWindow.cpp
    StarNetRunner.h
    StarNetRunner.cpp
    resources/starnet.rc
)

set(RESOURCES resources/icons.qrc)

target_link_libraries(StarNetGUI PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)
