cmake_minimum_required(VERSION 3.16)
project(FairWindSK VERSION 0.1.0 LANGUAGES CXX)

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(FAIRWINDSK_MOBILE FALSE)
if(ANDROID OR IOS)
    set(FAIRWINDSK_MOBILE TRUE)
endif()

find_package(Qt6 REQUIRED COMPONENTS
        Core
        Gui
        Widgets
        Concurrent
        Network
        WebSockets
        Xml
        Svg
        Positioning
        WebEngineWidgets
        VirtualKeyboard
)

set(EXTERNAL_INSTALL_LOCATION ${CMAKE_BINARY_DIR}/external)
include_directories(SYSTEM ${EXTERNAL_INSTALL_LOCATION}/include)
link_directories(${EXTERNAL_INSTALL_LOCATION}/lib)

set(FAIRWINDSK_LINK_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Concurrent
        Qt::Widgets
        Qt::Network
        Qt::WebSockets
        Qt::Xml
        Qt::Svg
        Qt::Positioning
        Qt::WebEngineWidgets
        Qt::VirtualKeyboard
)

set(FAIRWINDSK_POST_BUILD_COMMANDS)

set(FAIRWINDSK_EXTERNAL_DEPENDENCIES json)

include(ExternalProject)

ExternalProject_Add(json
        GIT_TAG v3.11.3
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${EXTERNAL_INSTALL_LOCATION} -DBUILD_TESTING=OFF -DJSON_BuildTests=OFF
        TIMEOUT 360
)

if(NOT FAIRWINDSK_MOBILE)
    find_package(Qt6 REQUIRED COMPONENTS PrintSupport)
    list(APPEND FAIRWINDSK_LINK_LIBRARIES Qt::PrintSupport)

    ExternalProject_Add(QtZeroConf
            GIT_REPOSITORY https://github.com/OpenFairWind/QtZeroConf.git
            CMAKE_ARGS -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH} -DCMAKE_INSTALL_PREFIX=${EXTERNAL_INSTALL_LOCATION} -DBUILD_SHARED_LIBS=ON
            TIMEOUT 360
    )

    if(WIN32)
        set(LIBQTZEROCONF ${EXTERNAL_INSTALL_LOCATION}/${CMAKE_INSTALL_LIBDIR}/${CMAKE_IMPORT_LIBRARY_PREFIX}QtZeroConf${CMAKE_IMPORT_LIBRARY_SUFFIX})
        set(RUNTIMEQTZEROCONF ${EXTERNAL_INSTALL_LOCATION}/${CMAKE_INSTALL_BINDIR}/${CMAKE_SHARED_LIBRARY_PREFIX}QtZeroConf${CMAKE_SHARED_LIBRARY_SUFFIX})
    else()
        set(LIBQTZEROCONF ${EXTERNAL_INSTALL_LOCATION}/${CMAKE_INSTALL_LIBDIR}/${CMAKE_SHARED_LIBRARY_PREFIX}QtZeroConf${CMAKE_SHARED_LIBRARY_SUFFIX})
    endif()

    ExternalProject_Add(qhotkey
            GIT_REPOSITORY https://github.com/Skycoder42/QHotkey.git
            CMAKE_ARGS -DBUILD_SHARED_LIBS=on -DCMAKE_INSTALL_PREFIX=${EXTERNAL_INSTALL_LOCATION} -DQT_DEFAULT_MAJOR_VERSION=6
            TIMEOUT 360
    )

    if(WIN32)
        set(LIBQHOTKEY ${EXTERNAL_INSTALL_LOCATION}/${CMAKE_INSTALL_LIBDIR}/${CMAKE_IMPORT_LIBRARY_PREFIX}qhotkey${CMAKE_IMPORT_LIBRARY_SUFFIX})
        set(RUNTIMEQHOTKEY ${EXTERNAL_INSTALL_LOCATION}/${CMAKE_INSTALL_BINDIR}/${CMAKE_SHARED_LIBRARY_PREFIX}qhotkey${CMAKE_SHARED_LIBRARY_SUFFIX})
    else()
        set(LIBQHOTKEY ${EXTERNAL_INSTALL_LOCATION}/${CMAKE_INSTALL_LIBDIR}/${CMAKE_SHARED_LIBRARY_PREFIX}qhotkey${CMAKE_SHARED_LIBRARY_SUFFIX})
    endif()

    list(APPEND FAIRWINDSK_EXTERNAL_DEPENDENCIES QtZeroConf qhotkey)
    list(APPEND FAIRWINDSK_LINK_LIBRARIES ${LIBQTZEROCONF} ${LIBQHOTKEY})

    if(WIN32)
        list(APPEND FAIRWINDSK_POST_BUILD_COMMANDS
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${RUNTIMEQTZEROCONF} $<TARGET_FILE_DIR:${PROJECT_NAME}>
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${RUNTIMEQHOTKEY} $<TARGET_FILE_DIR:${PROJECT_NAME}>)
    endif()

    if(APPLE AND TARGET Qt6::PrintSupport)
        get_target_property(_qt_print_support_libs Qt6::PrintSupport INTERFACE_LINK_LIBRARIES)
        set(_qt_print_support_resolved "${_qt_print_support_libs}")
        set(_missing_cups_libraries "")
        foreach(_lib IN LISTS _qt_print_support_libs)
            if(_lib MATCHES "libcups\\.(tbd|dylib)$" AND NOT EXISTS "${_lib}")
                list(APPEND _missing_cups_libraries "${_lib}")
            endif()
        endforeach()
        if(_missing_cups_libraries)
            find_library(CUPS_LIBRARY NAMES cups)
            if(CUPS_LIBRARY)
                list(REMOVE_ITEM _qt_print_support_resolved ${_missing_cups_libraries})
                list(APPEND _qt_print_support_resolved ${CUPS_LIBRARY})
                set_target_properties(Qt6::PrintSupport PROPERTIES INTERFACE_LINK_LIBRARIES "${_qt_print_support_resolved}")
                message(STATUS "Replaced missing Qt6::PrintSupport CUPS link with: ${CUPS_LIBRARY}")
            else()
                message(WARNING "Qt6::PrintSupport references missing CUPS library (${_missing_cups_libraries}) and no replacement could be found.")
            endif()
        endif()
    endif()
endif()

set(FAIRWINDSK_SOURCES
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
        ui/settings/Apps.cpp ui/settings/Apps.hpp ui/settings/Apps.ui
        ui/settings/Main.cpp ui/settings/Main.hpp ui/settings/Main.ui
        ui/settings/Connection.cpp ui/settings/Connection.hpp ui/settings/Connection.ui
        ui/settings/SignalK.cpp ui/settings/SignalK.hpp ui/settings/SignalK.ui
        Configuration.cpp Configuration.hpp
        ui/bottombar/POBBar.cpp ui/bottombar/POBBar.hpp ui/bottombar/POBBar.ui
        ui/bottombar/AlarmsBar.cpp ui/bottombar/AlarmsBar.hpp ui/bottombar/AlarmsBar.ui
        ui/web/NavigationBar.cpp ui/web/NavigationBar.hpp ui/web/NavigationBar.ui
        ui/bottombar/AutopilotBar.cpp ui/bottombar/AutopilotBar.hpp ui/bottombar/AutopilotBar.ui
        ui/mydata/MyData.cpp ui/mydata/MyData.hpp ui/mydata/MyData.ui
        ui/mydata/ResourceDialog.cpp ui/mydata/ResourceDialog.hpp
        ui/mydata/ResourceModel.cpp ui/mydata/ResourceModel.hpp
        ui/mydata/ResourceTab.cpp ui/mydata/ResourceTab.hpp
        ui/bottombar/AnchorBar.cpp ui/bottombar/AnchorBar.hpp ui/bottombar/AnchorBar.ui
        PList.cpp PList.hpp
        ui/mydata/Files.cpp ui/mydata/Files.hpp ui/mydata/Files.ui
        ui/mydata/FileInfoListModel.cpp ui/mydata/FileInfoListModel.hpp
        ui/mydata/FileViewer.cpp ui/mydata/FileViewer.hpp ui/mydata/FileViewer.ui
)

qt_add_executable(${PROJECT_NAME}
        MANUAL_FINALIZATION
        ${FAIRWINDSK_SOURCES}
)

set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        WIN32_EXECUTABLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "FairWindSK"
        MACOSX_BUNDLE_GUI_IDENTIFIER "org.openfairwind.fairwindsk"
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
)

if(ANDROID)
    set_property(TARGET ${PROJECT_NAME} PROPERTY QT_ANDROID_PACKAGE_NAME "org.openfairwind.fairwindsk")
endif()

foreach(_dependency IN LISTS FAIRWINDSK_EXTERNAL_DEPENDENCIES)
    add_dependencies(${PROJECT_NAME} ${_dependency})
endforeach()

target_link_libraries(${PROJECT_NAME} PRIVATE ${FAIRWINDSK_LINK_LIBRARIES})

if(NOT FAIRWINDSK_MOBILE)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/resources/images/icons $<TARGET_FILE_DIR:${PROJECT_NAME}>/icons/
            ${FAIRWINDSK_POST_BUILD_COMMANDS})
endif()

qt_finalize_executable(${PROJECT_NAME})
