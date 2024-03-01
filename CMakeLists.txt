cmake_minimum_required(VERSION 3.28)
project(FairWindSK)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_PREFIX_PATH "/usr/local/opt/qt6/")

# Include the support to external projects
#include(ExternalProject)

# Set the external install location
#set(EXTERNAL_INSTALL_LOCATION ${CMAKE_BINARY_DIR}/external)

# Add to the includes
#include_directories(SYSTEM ${EXTERNAL_INSTALL_LOCATION}/include)

# Add to the libraries
#link_directories(${EXTERNAL_INSTALL_LOCATION}/lib)

find_package(Qt6 COMPONENTS
        Core
        Gui
        Widgets
        Network
        WebSockets
        Xml
        Positioning
        WebEngineWidgets
        REQUIRED)

add_executable(${PROJECT_NAME}
        main.cpp
        FairWindSK.cpp FairWindSK.hpp
        AppItem.cpp AppItem.hpp
        ui/Launcher.cpp ui/Launcher.hpp ui/Launcher.cpp ui/Launcher.ui
        ui/MainWindow.cpp ui/MainWindow.hpp ui/MainWindow.ui
        ui/about/About.cpp ui/about/About.hpp ui/about/About.ui
        ui/topbar/TopBar.cpp ui/topbar/TopBar.hpp ui/topbar/TopBar.ui
        ui/bottombar/BottomBar.cpp ui/bottombar/BottomBar.hpp ui/bottombar/BottomBar.ui

        resources.qrc

)

target_link_libraries(${PROJECT_NAME}
        Qt::Core
        Qt::Gui
        Qt::Widgets
        Qt::Network
        Qt::WebSockets
        Qt::Xml
        Qt::Positioning
        Qt::WebEngineWidgets
)

