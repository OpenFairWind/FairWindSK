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

#### json https://github.com/nlohmann/json

ExternalProject_Add(json
        GIT_TAG v3.11.3
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${EXTERNAL_INSTALL_LOCATION} -DBUILD_TESTING=OFF -DJSON_BuildTests=OFF
        TIMEOUT 360
)

# QtZeroConf
ExternalProject_Add(QtZeroConf
        GIT_REPOSITORY https://github.com/jbagg/QtZeroConf.git
        CMAKE_ARGS -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH} -DCMAKE_INSTALL_PREFIX=${EXTERNAL_INSTALL_LOCATION} -DBUILD_SHARED_LIBS=ON
        TIMEOUT 360
)

set(LIBQTZEROCONF ${EXTERNAL_INSTALL_LOCATION}/lib/libQtZeroConf.so)
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
        Units.cpp Units.hpp
        FairWindSK.cpp FairWindSK.hpp
        AppItem.cpp AppItem.hpp

        signalk/Note.cpp signalk/Note.hpp
        signalk/Waypoint.cpp signalk/Waypoint.hpp
        signalk/Subscription.cpp signalk/Subscription.hpp
        signalk/Client.cpp signalk/Client.hpp

        ui/settings/Settings.cpp ui/settings/Settings.hpp ui/settings/Settings.ui

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
        ui/settings/Apps.cpp
        ui/settings/Apps.hpp
        ui/settings/Apps.ui
        ui/settings/Main.cpp
        ui/settings/Main.hpp
        ui/settings/Main.ui
        ui/settings/Connection.cpp
        ui/settings/Connection.hpp
        ui/settings/Connection.ui
        ui/settings/SignalK.cpp
        ui/settings/SignalK.hpp
        ui/settings/SignalK.ui
        Configuration.cpp
        Configuration.hpp
        ui/bottombar/MOBBar.cpp
        ui/bottombar/MOBBar.hpp
        ui/bottombar/MOBBar.ui
        ui/bottombar/AlarmsBar.cpp
        ui/bottombar/AlarmsBar.hpp
        ui/bottombar/AlarmsBar.ui
        ui/web/NavigationBar.cpp
        ui/web/NavigationBar.hpp
        ui/web/NavigationBar.ui
        ui/bottombar/AutopilotBar.cpp
        ui/bottombar/AutopilotBar.hpp
        ui/bottombar/AutopilotBar.ui
        ui/mydata/MyData.cpp
        ui/mydata/MyData.hpp
        ui/mydata/MyData.ui
        ui/bottombar/AnchorBar.cpp
        ui/bottombar/AnchorBar.hpp
        ui/bottombar/AnchorBar.ui

)

add_dependencies(${PROJECT_NAME} json)
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

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/resources/images/icons $<TARGET_FILE_DIR:${PROJECT_NAME}>/icons/)
