cmake_minimum_required(VERSION 3.25)
project(FairWindSK)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_PREFIX_PATH "/usr/local/opt/qt6/")

# Include the support to external projects
include(ExternalProject)

# Set the external install location
set(EXTERNAL_INSTALL_LOCATION ${CMAKE_BINARY_DIR}/external)

# Add to the includes
include_directories(SYSTEM ${EXTERNAL_INSTALL_LOCATION}/include)

# Add to the libraries
link_directories(${EXTERNAL_INSTALL_LOCATION}/lib)

ExternalProject_Add(QtZeroConf
        GIT_REPOSITORY https://github.com/jbagg/QtZeroConf.git
        CMAKE_ARGS -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH} -DCMAKE_INSTALL_PREFIX=${EXTERNAL_INSTALL_LOCATION} -DBUILD_SHARED_LIBS=ON
        TIMEOUT 360
)
set(LIBQTZEROCONF ${EXTERNAL_INSTALL_LOCATION}/lib/libqtzeroconf.so)
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(LIBQTZEROCONF ${EXTERNAL_INSTALL_LOCATION}/lib/static/libQtZeroConf.lib)
    add_custom_command(
            TARGET QtZeroConf
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            ${EXTERNAL_INSTALL_LOCATION}/bin/libQtZeroConf.dll
            ${CMAKE_BINARY_DIR}
    )
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(LIBQTZEROCONF ${EXTERNAL_INSTALL_LOCATION}/lib/libQtZeroConf.dylib)
endif()
set(LIBQTZEROCONF ${LIBQTZEROCONF})

find_package(Qt6 COMPONENTS
        Core
        Gui
        Widgets
        Network
        WebSockets
        Xml
        Positioning
        WebEngineWidgets
        VirtualKeyboard
        REQUIRED)

add_executable(${PROJECT_NAME}
        main.cpp
        IterableLayoutAdapter.hpp
        FairWindSK.cpp FairWindSK.hpp
        AppItem.cpp AppItem.hpp
        SignalKClient.cpp SignalKClient.hpp
        signalk/Document.cpp signalk/Document.hpp
        signalk/Note.cpp signalk/Note.hpp
        signalk/Waypoint.cpp signalk/Waypoint.hpp
        signalk/Subscription.cpp signalk/Subscription.hpp

        ui/launcher/Launcher.cpp ui/launcher/Launcher.hpp ui/launcher/Launcher.ui
        ui/about/About.cpp ui/about/About.hpp ui/about/About.ui
        ui/topbar/TopBar.cpp ui/topbar/TopBar.hpp ui/topbar/TopBar.ui
        ui/bottombar/BottomBar.cpp ui/bottombar/BottomBar.hpp ui/bottombar/BottomBar.ui
        ui/MainWindow.cpp ui/MainWindow.hpp ui/MainWindow.ui

        ui/web/Web.cpp ui/web/Web.hpp ui/web/Web.ui
        ui/web/WebPage.cpp ui/web/WebPage.hpp
        ui/web/WebPopupWindow.cpp ui/web/WebPopupWindow.hpp
        ui/web/WebView.cpp ui/web/WebView.hpp
        ui/web/DownloadManagerWidget.cpp ui/web/DownloadManagerWidget.hpp ui/web/DownloadManagerWidget.ui
        ui/web/DownloadWidget.cpp ui/web/DownloadWidget.hpp ui/web/DownloadWidget.ui

        resources.qrc
        ui/Settings.cpp
        ui/Settings.hpp
        ui/Settings.ui
)

add_dependencies(${PROJECT_NAME} QtZeroConf)

target_link_libraries(${PROJECT_NAME}
        Qt::Core
        Qt::Gui
        Qt::Widgets
        Qt::Network
        Qt::WebSockets
        Qt::Xml
        Qt::Positioning
        Qt::WebEngineWidgets
        Qt::VirtualKeyboard
        ${LIBQTZEROCONF}
)

