cmake_minimum_required(VERSION 3.16)

# Keep the public SemVer string in one plain-text file so CMake, CI, and release
# tooling cannot silently disagree about the version being shipped.
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION.txt" FAIRWINDSK_SEMVER LIMIT_COUNT 1)
if(NOT FAIRWINDSK_SEMVER MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+(-[0-9A-Za-z.-]+)?(\\+[0-9A-Za-z.-]+)?$")
    message(FATAL_ERROR "VERSION.txt must contain a valid semantic version")
endif()
string(REGEX MATCH "^[0-9]+\\.[0-9]+\\.[0-9]+" FAIRWINDSK_PROJECT_VERSION "${FAIRWINDSK_SEMVER}")
project(FairWindSK VERSION ${FAIRWINDSK_PROJECT_VERSION} LANGUAGES CXX)

include(CTest)

# ==========================================
# 1. LOGICA DI VERSIONAMENTO (GIT)
# ==========================================
execute_process(
        COMMAND git -C ${CMAKE_SOURCE_DIR} describe --tags --always --dirty
        OUTPUT_VARIABLE FAIRWINDSK_GIT_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
execute_process(
        COMMAND git -C ${CMAKE_SOURCE_DIR} rev-list --count HEAD
        OUTPUT_VARIABLE FAIRWINDSK_GIT_COMMIT_COUNT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
execute_process(
        COMMAND git -C ${CMAKE_SOURCE_DIR} rev-parse --abbrev-ref HEAD
        OUTPUT_VARIABLE FAIRWINDSK_GIT_BRANCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)

if(FAIRWINDSK_GIT_COMMIT_COUNT MATCHES "^[0-9]+$")
    math(EXPR FAIRWINDSK_ANDROID_VERSION_CODE "${FAIRWINDSK_GIT_COMMIT_COUNT} + 1")
else()
    set(FAIRWINDSK_ANDROID_VERSION_CODE 1)
endif()

if(NOT FAIRWINDSK_GIT_VERSION)
    set(FAIRWINDSK_GIT_VERSION ${FAIRWINDSK_SEMVER})
endif()
if(NOT FAIRWINDSK_GIT_BRANCH)
    set(FAIRWINDSK_GIT_BRANCH "unknown")
endif()
string(TIMESTAMP FAIRWINDSK_BUILD_YEAR "%Y")

include(GNUInstallDirs)

# ==========================================
# 2. CONFIGURAZIONE TARGET E COMPILATORE
# ==========================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Definiamo la piattaforma mobile
set(FAIRWINDSK_MOBILE FALSE)
if(ANDROID OR IOS)
    set(FAIRWINDSK_MOBILE TRUE)
endif()

# SE SIAMO SU WINDOWS MA CON MIN GW, EMETTIAMO UN MESSAGGIO BLOCCANTE CHIARO
if(MINGW AND NOT FAIRWINDSK_MOBILE)
    message(FATAL_ERROR "Compilatore MinGW rilevato su ambiente Windows Desktop. L'architettura Desktop di FairWindSK richiede nativamente il modulo Qt WebEngine (Chromium), non supportato da MinGW. Selezionare il Kit Android per compilare la versione mobile del progetto.")
endif()

# ==========================================
# 3. RICERCA COMPONENTI QT6
# ==========================================
find_package(Qt6 REQUIRED COMPONENTS
        Core Gui Widgets Qml Concurrent Network WebSockets Xml Svg SvgWidgets Positioning VirtualKeyboard LinguistTools
)

if(FAIRWINDSK_MOBILE)
    find_package(Qt6 REQUIRED COMPONENTS Quick QuickWidgets WebView)
else()
    find_package(Qt6 REQUIRED COMPONENTS WebEngineWidgets PrintSupport)
endif()

# Build host-runnable unit tests only for desktop kits. Mobile binaries require a
# device or simulator and therefore cannot be executed directly by host CTest.
if(BUILD_TESTING AND NOT FAIRWINDSK_MOBILE)
    find_package(Qt6 REQUIRED COMPONENTS Test)
endif()

# Qt 6.8.3's native arm64 host tools abort on some Apple Silicon hosts with an
# incorrect NEON feature check. Their universal x86_64 slices work under Rosetta
# while continuing to generate arm64 Android and x86_64 Simulator target output.
if(ANDROID AND CMAKE_HOST_APPLE AND CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "^(arm64|aarch64)$"
        AND Qt6_VERSION VERSION_EQUAL "6.8.3")
    set(FAIRWINDSK_ORIGINAL_QT_HOST_PATH "${QT_HOST_PATH}")
    set(FAIRWINDSK_ROSETTA_QT_HOST_PATH "${CMAKE_BINARY_DIR}/fairwindsk-qt-host")
    file(MAKE_DIRECTORY "${FAIRWINDSK_ROSETTA_QT_HOST_PATH}/bin")
    file(WRITE "${FAIRWINDSK_ROSETTA_QT_HOST_PATH}/bin/androiddeployqt"
        "#!/bin/sh\nexec /usr/bin/arch -x86_64 \"${QT_HOST_PATH}/${QT6_HOST_INFO_BINDIR}/androiddeployqt\" \"$@\"\n")
    file(CHMOD "${FAIRWINDSK_ROSETTA_QT_HOST_PATH}/bin/androiddeployqt"
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

    function(fairwindsk_use_rosetta_qt_tool target_name)
        if(NOT TARGET ${target_name})
            return()
        endif()
        get_target_property(tool_path ${target_name} IMPORTED_LOCATION)
        if(NOT tool_path)
            return()
        endif()
        string(REPLACE "Qt6::" "" tool_name "${target_name}")
        set(wrapper_path "${CMAKE_BINARY_DIR}/fairwindsk-${tool_name}")
        file(WRITE "${wrapper_path}"
            "#!/bin/sh\nexec /usr/bin/arch -x86_64 \"${tool_path}\" \"$@\"\n")
        file(CHMOD "${wrapper_path}"
            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
        set_property(TARGET ${target_name} PROPERTY IMPORTED_LOCATION "${wrapper_path}")
    endfunction()

    foreach(tool_target IN ITEMS
            Qt6::moc Qt6::uic Qt6::rcc Qt6::lrelease Qt6::lupdate
            Qt6::qmlimportscanner Qt6::qmlcachegen Qt6::qmltyperegistrar
            Qt6::androiddeployqt)
        fairwindsk_use_rosetta_qt_tool(${tool_target})
    endforeach()
endif()

# ==========================================
# 4. PATH DIPENDENZE ESTERNE
# ==========================================
set(EXTERNAL_INSTALL_LOCATION ${CMAKE_BINARY_DIR}/external)
include_directories(SYSTEM ${EXTERNAL_INSTALL_LOCATION}/include)
link_directories(${EXTERNAL_INSTALL_LOCATION}/lib)

set(FAIRWINDSK_LINK_LIBRARIES
        Qt::Core Qt::Gui Qt::Concurrent Qt::Widgets Qt::Qml Qt::Network Qt::WebSockets Qt::Xml Qt::Svg Qt::SvgWidgets Qt::Positioning Qt::VirtualKeyboard
)

if(FAIRWINDSK_MOBILE)
    list(APPEND FAIRWINDSK_LINK_LIBRARIES Qt::Quick Qt::QuickWidgets Qt::WebView)
else()
    list(APPEND FAIRWINDSK_LINK_LIBRARIES Qt::WebEngineWidgets Qt::PrintSupport)
endif()

set(FAIRWINDSK_POST_BUILD_COMMANDS)
set(FAIRWINDSK_EXTERNAL_DEPENDENCIES)

include(ExternalProject)

# nlohmann_json
find_package(nlohmann_json CONFIG QUIET)
if(NOT nlohmann_json_FOUND)
    find_path(NLOHMANN_JSON_SYSTEM_INCLUDE_DIR NAMES nlohmann/json.hpp PATH_SUFFIXES include)
    if(NLOHMANN_JSON_SYSTEM_INCLUDE_DIR)
        add_library(nlohmann_json INTERFACE)
        add_library(nlohmann_json::nlohmann_json ALIAS nlohmann_json)
        target_include_directories(nlohmann_json INTERFACE ${NLOHMANN_JSON_SYSTEM_INCLUDE_DIR})
    else()
        set(NLOHMANN_JSON_INCLUDE_DIR ${CMAKE_BINARY_DIR}/third_party/nlohmann)
        set(NLOHMANN_JSON_HEADER ${NLOHMANN_JSON_INCLUDE_DIR}/nlohmann/json.hpp)
        set(NLOHMANN_JSON_SHA256 9bea4c8066ef4a1c206b2be5a36302f8926f7fdc6087af5d20b417d0cf103ea6)
        file(MAKE_DIRECTORY ${NLOHMANN_JSON_INCLUDE_DIR}/nlohmann)
        if(EXISTS ${NLOHMANN_JSON_HEADER})
            file(SHA256 ${NLOHMANN_JSON_HEADER} NLOHMANN_JSON_ACTUAL_SHA256)
        endif()
        if(NOT NLOHMANN_JSON_ACTUAL_SHA256 STREQUAL NLOHMANN_JSON_SHA256)
            file(DOWNLOAD
                    https://raw.githubusercontent.com/nlohmann/json/v3.11.3/single_include/nlohmann/json.hpp
                    ${NLOHMANN_JSON_HEADER}
                    EXPECTED_HASH SHA256=${NLOHMANN_JSON_SHA256}
                    TLS_VERIFY ON
                    TIMEOUT 120
            )
        endif()
        add_library(nlohmann_json INTERFACE)
        add_library(nlohmann_json::nlohmann_json ALIAS nlohmann_json)
        target_include_directories(nlohmann_json INTERFACE ${CMAKE_BINARY_DIR}/third_party/nlohmann)
    endif()
endif()
list(APPEND FAIRWINDSK_LINK_LIBRARIES nlohmann_json::nlohmann_json)

# Sotto-progetti esterni (Esclusi su Mobile per velocizzare e ripulire i pacchetti)
if(NOT FAIRWINDSK_MOBILE)
    if(WIN32)
        set(LIBQTZEROCONF ${EXTERNAL_INSTALL_LOCATION}/${CMAKE_INSTALL_LIBDIR}/${CMAKE_IMPORT_LIBRARY_PREFIX}QtZeroConf${CMAKE_IMPORT_LIBRARY_SUFFIX})
        set(RUNTIMEQTZEROCONF ${EXTERNAL_INSTALL_LOCATION}/${CMAKE_INSTALL_BINDIR}/${CMAKE_SHARED_LIBRARY_PREFIX}QtZeroConf${CMAKE_SHARED_LIBRARY_SUFFIX})
    else()
        set(LIBQTZEROCONF ${EXTERNAL_INSTALL_LOCATION}/${CMAKE_INSTALL_LIBDIR}/${CMAKE_SHARED_LIBRARY_PREFIX}QtZeroConf${CMAKE_SHARED_LIBRARY_SUFFIX})
    endif()

    if(WIN32)
        set(LIBQHOTKEY ${EXTERNAL_INSTALL_LOCATION}/${CMAKE_INSTALL_LIBDIR}/${CMAKE_IMPORT_LIBRARY_PREFIX}qhotkey${CMAKE_IMPORT_LIBRARY_SUFFIX})
        set(RUNTIMEQHOTKEY ${EXTERNAL_INSTALL_LOCATION}/${CMAKE_INSTALL_BINDIR}/${CMAKE_SHARED_LIBRARY_PREFIX}qhotkey${CMAKE_SHARED_LIBRARY_SUFFIX})
    else()
        set(LIBQHOTKEY ${EXTERNAL_INSTALL_LOCATION}/${CMAKE_INSTALL_LIBDIR}/${CMAKE_SHARED_LIBRARY_PREFIX}qhotkey${CMAKE_SHARED_LIBRARY_SUFFIX})
    endif()

    if(EXISTS "${LIBQTZEROCONF}" AND EXISTS "${LIBQHOTKEY}")
        message(STATUS "Using previously installed QtZeroConf and QHotkey from ${EXTERNAL_INSTALL_LOCATION}")
    else()
        ExternalProject_Add(QtZeroConf
                GIT_REPOSITORY https://github.com/OpenFairWind/QtZeroConf.git
                GIT_TAG f7744716dff7fad22ca9edd4e3182477afe43b8d
                CMAKE_ARGS -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH} -DCMAKE_INSTALL_PREFIX=${EXTERNAL_INSTALL_LOCATION} -DBUILD_SHARED_LIBS=ON -DCMAKE_POLICY_VERSION_MINIMUM=3.16 -DCMAKE_INSTALL_REMOVE_ENVIRONMENT_RPATH=FALSE "-DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}"
                BUILD_BYPRODUCTS ${LIBQTZEROCONF}
                TIMEOUT 360
        )
        ExternalProject_Add(qhotkey
                GIT_REPOSITORY https://github.com/Skycoder42/QHotkey.git
                GIT_TAG 1.5.0
                CMAKE_ARGS -DBUILD_SHARED_LIBS=on -DCMAKE_INSTALL_PREFIX=${EXTERNAL_INSTALL_LOCATION} -DQT_DEFAULT_MAJOR_VERSION=6 -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_INSTALL_REMOVE_ENVIRONMENT_RPATH=FALSE -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH} "-DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}"
                BUILD_BYPRODUCTS ${LIBQHOTKEY}
                TIMEOUT 360
        )
        list(APPEND FAIRWINDSK_EXTERNAL_DEPENDENCIES QtZeroConf qhotkey)
    endif()
    list(APPEND FAIRWINDSK_LINK_LIBRARIES ${LIBQTZEROCONF} ${LIBQHOTKEY})
endif()

# ==========================================
# 5. ELENCO FILE SORGENTI
# ==========================================
set(FAIRWINDSK_SOURCES
        main.cpp
        Units.cpp Units.hpp
        FairWindSK.cpp FairWindSK.hpp
        Localization.cpp Localization.hpp
        AppItem.cpp AppItem.hpp
        signalk/Note.cpp signalk/Note.hpp
        signalk/Waypoint.cpp signalk/Waypoint.hpp
        signalk/Subscription.cpp signalk/Subscription.hpp
        signalk/ProtocolUtils.cpp signalk/ProtocolUtils.hpp
        signalk/Client.cpp signalk/Client.hpp
        runtime/DiagnosticsSupport.cpp runtime/DiagnosticsSupport.hpp
        runtime/ManualScreenshotCapture.cpp runtime/ManualScreenshotCapture.hpp
        ui/layout/BarLayout.cpp ui/layout/BarLayout.hpp
        ui/layout/BarRuntime.cpp ui/layout/BarRuntime.hpp
        ui/settings/Settings.cpp ui/settings/Settings.hpp ui/settings/Settings.ui
        ui/settings/BarLayoutSettings.cpp ui/settings/BarLayoutSettings.hpp
        ui/settings/DataWidgets.cpp ui/settings/DataWidgets.hpp
        ui/settings/LayoutPreviewListWidget.cpp ui/settings/LayoutPreviewListWidget.hpp
        ui/settings/WidgetPalette.cpp ui/settings/WidgetPalette.hpp
        ui/launcher/Launcher.cpp ui/launcher/Launcher.hpp ui/launcher/Launcher.ui
        ui/about/About.cpp ui/about/About.hpp ui/about/About.ui
        ui/topbar/TopBar.cpp ui/topbar/TopBar.hpp ui/topbar/TopBar.ui
        ui/bottombar/BottomBar.cpp ui/bottombar/BottomBar.hpp ui/bottombar/BottomBar.ui
        ui/MainWindow.cpp ui/MainWindow.hpp ui/MainWindow.ui
        ui/DrawerDialogHost.cpp ui/DrawerDialogHost.hpp
        ui/GeoCoordinateUtils.cpp ui/GeoCoordinateUtils.hpp
        ui/GeoCoordinateEditorWidget.cpp ui/GeoCoordinateEditorWidget.hpp ui/GeoCoordinateEditorWidget.ui
        ui/widgets/TouchComboBox.cpp ui/widgets/TouchComboBox.hpp ui/widgets/TouchComboBox.ui
        ui/widgets/TouchCheckBox.cpp ui/widgets/TouchCheckBox.hpp ui/widgets/TouchCheckBox.ui
        ui/widgets/TouchColorPicker.cpp ui/widgets/TouchColorPicker.hpp
        ui/widgets/TouchFileBrowser.cpp ui/widgets/TouchFileBrowser.hpp
        ui/widgets/TouchIconBrowser.cpp ui/widgets/TouchIconBrowser.hpp
        ui/widgets/TouchScrollArea.cpp ui/widgets/TouchScrollArea.hpp
        ui/widgets/TouchSpinBox.cpp ui/widgets/TouchSpinBox.hpp ui/widgets/TouchSpinBox.ui
        ui/widgets/SignalKMetricState.hpp
        ui/widgets/DataWidgetConfig.cpp ui/widgets/DataWidgetConfig.hpp
        ui/widgets/DataWidget.cpp ui/widgets/DataWidget.hpp
        ui/widgets/SignalKStatusIconsWidget.cpp ui/widgets/SignalKStatusIconsWidget.hpp
        ui/widgets/SignalKServerBox.cpp ui/widgets/SignalKServerBox.hpp ui/widgets/SignalKServerBox.ui
        ui/web/Web.cpp ui/web/Web.hpp ui/web/Web.ui
        ui/web/WebView.cpp ui/web/WebView.hpp
        ui/web/SignalKAppView.cpp ui/web/SignalKAppView.hpp ui/web/SignalKAppView.ui
        resources.qrc
        ui/settings/Apps.cpp ui/settings/Apps.hpp ui/settings/Apps.ui
        ui/settings/AppDetailsWidget.cpp ui/settings/AppDetailsWidget.hpp ui/settings/AppDetailsWidget.ui
        ui/settings/PageDetailsWidget.cpp ui/settings/PageDetailsWidget.hpp ui/settings/PageDetailsWidget.ui
        ui/settings/Main.cpp ui/settings/Main.hpp ui/settings/Main.ui
        ui/settings/Comfort.cpp ui/settings/Comfort.hpp
        ui/settings/Connection.cpp ui/settings/Connection.hpp ui/settings/Connection.ui
        ui/settings/SignalK.cpp ui/settings/SignalK.hpp ui/settings/SignalK.ui
        ui/settings/Units.cpp ui/settings/Units.hpp ui/settings/Units.ui
        ui/settings/System.cpp ui/settings/System.hpp ui/settings/System.ui
        ui/settings/AndroidApps.cpp ui/settings/AndroidApps.hpp
        platform/android/AndroidApplicationBridge.cpp platform/android/AndroidApplicationBridge.hpp
        Configuration.cpp Configuration.hpp
        ui/bottombar/POBBar.cpp ui/bottombar/POBBar.hpp ui/bottombar/POBBar.ui
        ui/bottombar/AlarmsBar.cpp ui/bottombar/AlarmsBar.hpp ui/bottombar/AlarmsBar.ui
        ui/web/NavigationBar.cpp ui/web/NavigationBar.hpp ui/web/NavigationBar.ui
        ui/bottombar/AutopilotBar.cpp ui/bottombar/AutopilotBar.hpp ui/bottombar/AutopilotBar.ui
        ui/mydata/MyData.cpp ui/mydata/MyData.hpp ui/mydata/MyData.ui
        ui/mydata/Waypoints.cpp ui/mydata/Waypoints.hpp ui/mydata/Waypoints.ui
        ui/mydata/Routes.cpp ui/mydata/Routes.hpp ui/mydata/Routes.ui
        ui/mydata/Regions.cpp ui/mydata/Regions.hpp ui/mydata/Regions.ui
        ui/mydata/Notes.cpp ui/mydata/Notes.hpp ui/mydata/Notes.ui
        ui/mydata/Charts.cpp ui/mydata/Charts.hpp ui/mydata/Charts.ui
        ui/mydata/GeoJsonUtils.cpp ui/mydata/GeoJsonUtils.hpp
        ui/mydata/GeoJsonPreviewWidget.cpp ui/mydata/GeoJsonPreviewWidget.hpp ui/mydata/GeoJsonPreviewWidget.ui
        ui/mydata/JsonObjectEditorWidget.cpp ui/mydata/JsonObjectEditorWidget.hpp ui/mydata/JsonObjectEditorWidget.ui
        ui/mydata/ResourceCollectionPageBase.cpp ui/mydata/ResourceCollectionPageBase.hpp
        ui/mydata/ResourceDialog.cpp ui/mydata/ResourceDialog.hpp ui/mydata/ResourceDialog.ui
        ui/mydata/ResourceModel.cpp ui/mydata/ResourceModel.hpp
        ui/mydata/ResourceTab.cpp ui/mydata/ResourceTab.hpp ui/mydata/ResourceTab.ui
        ui/mydata/HistoryTrackModel.cpp ui/mydata/HistoryTrackModel.hpp
        ui/mydata/HistoryTrackTab.cpp ui/mydata/HistoryTrackTab.hpp ui/mydata/HistoryTrackTab.ui
        ui/bottombar/AnchorBar.cpp ui/bottombar/AnchorBar.hpp ui/bottombar/AnchorBar.ui
        PList.cpp PList.hpp
        ui/mydata/Files.cpp ui/mydata/Files.hpp ui/mydata/Files.ui
        ui/mydata/FileInfoListModel.cpp ui/mydata/FileInfoListModel.hpp
        ui/mydata/FileViewer.cpp ui/mydata/FileViewer.hpp ui/mydata/FileViewer.ui
)

if(NOT FAIRWINDSK_MOBILE)
    list(APPEND FAIRWINDSK_SOURCES
            ui/web/WebPage.cpp ui/web/WebPage.hpp
            ui/web/DownloadManagerWidget.cpp ui/web/DownloadManagerWidget.hpp ui/web/DownloadManagerWidget.ui
            ui/web/DownloadWidget.cpp ui/web/DownloadWidget.hpp ui/web/DownloadWidget.ui
    )
endif()

# ==========================================
# 6. TARGET EXECUTABLE & CORES
# ==========================================
qt_add_executable(${PROJECT_NAME} MANUAL_FINALIZATION ${FAIRWINDSK_SOURCES})

qt_add_translations(${PROJECT_NAME} TS_FILES resources/i18n/fairwindsk_it.ts RESOURCE_PREFIX /resources/i18n)

set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE WIN32_EXECUTABLE TRUE MACOSX_BUNDLE_BUNDLE_NAME "FairWindSK"
        MACOSX_BUNDLE_GUI_IDENTIFIER "org.openfairwind.fairwindsk" MACOSX_BUNDLE_BUNDLE_VERSION ${FAIRWINDSK_SEMVER} MACOSX_BUNDLE_SHORT_VERSION_STRING ${FAIRWINDSK_SEMVER}
)

if(UNIX AND NOT APPLE AND NOT FAIRWINDSK_MOBILE)
    # Resolve the two bundled project libraries relative to the installed
    # executable without relying on a machine-specific global library path.
    set_target_properties(${PROJECT_NAME} PROPERTIES INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}")
endif()

if(IOS)
    # Keep the marine MFD shell in its intended wide layout on both iPhone and iPadOS.
    set(FAIRWINDSK_IOS_ASSET_CATALOG "${CMAKE_CURRENT_SOURCE_DIR}/apple/Assets.xcassets")
    target_sources(${PROJECT_NAME} PRIVATE "${FAIRWINDSK_IOS_ASSET_CATALOG}")
    set_source_files_properties("${FAIRWINDSK_IOS_ASSET_CATALOG}" PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources
    )
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/apple/Info.plist.in"
        XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME "AppIcon"
    )
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE
        FAIRWINDSK_VERSION="${FAIRWINDSK_SEMVER}" FAIRWINDSK_BUILD_YEAR="${FAIRWINDSK_BUILD_YEAR}" FAIRWINDSK_GIT_BRANCH="${FAIRWINDSK_GIT_BRANCH}"
)

if(WIN32 AND NOT FAIRWINDSK_MOBILE)
    # Import QHotkey's exported Qt meta-object data from its shared Windows DLL.
    target_compile_definitions(${PROJECT_NAME} PRIVATE QHOTKEY_SHARED)
endif()

if(ANDROID)
    if(COMMAND qt_policy)
        qt_policy(SET QTP0002 OLD)
    endif()

    set_target_properties(${PROJECT_NAME} PROPERTIES
        QT_ANDROID_APP_ICON "@mipmap/ic_launcher"
        QT_ANDROID_APP_NAME "FairWindSK"
        QT_ANDROID_PACKAGE_NAME "org.openfairwind.fairwindsk"
        QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android"
        QT_ANDROID_MIN_SDK_VERSION 33
        QT_ANDROID_VERSION_CODE "${FAIRWINDSK_ANDROID_VERSION_CODE}"
        QT_ANDROID_VERSION_NAME "${FAIRWINDSK_SEMVER}"
    )

    # Pin the Android TLS helper so clean builds remain reproducible.
    include(FetchContent)
    FetchContent_Declare(
        android_openssl
        GIT_REPOSITORY https://github.com/KDAB/android_openssl.git
        GIT_TAG b71f1470962019bd89534a2919f5925f93bc5779
        GIT_SHALLOW FALSE
    )
    FetchContent_MakeAvailable(android_openssl)
    include(${android_openssl_SOURCE_DIR}/android_openssl.cmake)

    add_android_openssl_libraries(${PROJECT_NAME})
endif()
foreach(_dependency IN LISTS FAIRWINDSK_EXTERNAL_DEPENDENCIES)
    add_dependencies(${PROJECT_NAME} ${_dependency})
endforeach()

target_link_libraries(${PROJECT_NAME} PRIVATE ${FAIRWINDSK_LINK_LIBRARIES})
if(FAIRWINDSK_MOBILE)
    target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::WebView)
endif()

# Keep test executables independent from the application target so CTest can run
# them headlessly on every supported desktop platform, including Windows.
if(BUILD_TESTING AND NOT FAIRWINDSK_MOBILE)
    function(fairwindsk_add_unit_test target_name test_name)
        qt_add_executable(${target_name} ${ARGN})
        target_link_libraries(${target_name} PRIVATE Qt6::Core Qt6::Test)
        add_test(NAME ${test_name} COMMAND ${target_name})
    endfunction()

    fairwindsk_add_unit_test(FairWindSKGeoCoordinateUtilsTest GeoCoordinateUtils
            tests/GeoCoordinateUtilsTest.cpp
            ui/GeoCoordinateUtils.cpp ui/GeoCoordinateUtils.hpp)
    target_link_libraries(FairWindSKGeoCoordinateUtilsTest PRIVATE Qt6::Positioning)

    fairwindsk_add_unit_test(FairWindSKPListTest PList
            tests/PListTest.cpp
            PList.cpp PList.hpp)
    target_link_libraries(FairWindSKPListTest PRIVATE Qt6::Xml)

    fairwindsk_add_unit_test(FairWindSKSignalKValueObjectsTest SignalKValueObjects
            tests/SignalKValueObjectsTest.cpp
            signalk/Waypoint.cpp signalk/Waypoint.hpp
            signalk/Subscription.cpp signalk/Subscription.hpp)
    target_link_libraries(FairWindSKSignalKValueObjectsTest PRIVATE Qt6::Positioning)

    fairwindsk_add_unit_test(FairWindSKSignalKProtocolTest SignalKProtocol
            tests/SignalKProtocolTest.cpp
            signalk/ProtocolUtils.cpp signalk/ProtocolUtils.hpp)

    fairwindsk_add_unit_test(FairWindSKSimulatedSignalKServiceTest SimulatedSignalKService
            tests/SimulatedSignalKServiceTest.cpp
            tests/support/SimulatedSignalKService.cpp tests/support/SimulatedSignalKService.hpp)
    target_link_libraries(FairWindSKSimulatedSignalKServiceTest PRIVATE Qt6::Network Qt6::WebSockets Qt6::Widgets)
    fairwindsk_add_unit_test(FairWindSKDataWidgetConfigTest DataWidgetConfig
            tests/DataWidgetConfigTest.cpp
            ui/widgets/DataWidgetConfig.cpp ui/widgets/DataWidgetConfig.hpp)
    target_link_libraries(FairWindSKDataWidgetConfigTest PRIVATE Qt6::Gui nlohmann_json::nlohmann_json)

    fairwindsk_add_unit_test(FairWindSKConfigurationTest Configuration
            tests/ConfigurationTest.cpp
            Configuration.cpp Configuration.hpp
            Localization.cpp Localization.hpp)
    target_link_libraries(FairWindSKConfigurationTest PRIVATE Qt6::Gui nlohmann_json::nlohmann_json)

    add_test(NAME UnitsReentrancy
            COMMAND $<TARGET_FILE:${PROJECT_NAME}> --self-test-units-reentrancy)
    add_test(NAME DirectorySafety
            COMMAND $<TARGET_FILE:${PROJECT_NAME}> --self-test-directory-safety)
    add_test(NAME ConfigurationImport
            COMMAND $<TARGET_FILE:${PROJECT_NAME}> --self-test-configuration-import)

    set_tests_properties(GeoCoordinateUtils PList SignalKValueObjects SignalKProtocol
            DataWidgetConfig Configuration UnitsReentrancy DirectorySafety ConfigurationImport
            PROPERTIES LABELS "unit")
    set_tests_properties(SimulatedSignalKService PROPERTIES LABELS "component;signalk")
endif()

# ==========================================
# 7. INSTALLAZIONE E FINALIZE
# ==========================================
if(FAIRWINDSK_ROSETTA_QT_HOST_PATH)
    # Qt's Android finalizer constructs androiddeployqt directly from QT_HOST_PATH.
    set(QT_HOST_PATH "${FAIRWINDSK_ROSETTA_QT_HOST_PATH}")
endif()
qt_finalize_executable(${PROJECT_NAME})
if(FAIRWINDSK_ORIGINAL_QT_HOST_PATH)
    set(QT_HOST_PATH "${FAIRWINDSK_ORIGINAL_QT_HOST_PATH}")
endif()

# ==========================================
# 8. INSTALLATION
# ==========================================
install(TARGETS ${PROJECT_NAME}
        BUNDLE DESTINATION .
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(NOT FAIRWINDSK_MOBILE)
    if(WIN32)
        install(FILES ${RUNTIMEQTZEROCONF} ${RUNTIMEQHOTKEY}
                DESTINATION ${CMAKE_INSTALL_BINDIR})
    elseif(APPLE)
        # Place non-Qt shared libraries where macdeployqt can discover and fix
        # their install names before the application bundle becomes a DMG.
        install(DIRECTORY "${EXTERNAL_INSTALL_LOCATION}/${CMAKE_INSTALL_LIBDIR}/"
                DESTINATION FairWindSK.app/Contents/Frameworks
                FILES_MATCHING PATTERN "*.dylib" PATTERN "cmake" EXCLUDE)
    elseif(UNIX AND NOT APPLE)
        # Install each shared library's versioned payload together with its linker symlinks.
        install(DIRECTORY "${EXTERNAL_INSTALL_LOCATION}/${CMAKE_INSTALL_LIBDIR}/"
                DESTINATION ${CMAKE_INSTALL_LIBDIR}
                FILES_MATCHING PATTERN "*.so*")

        configure_file(cmake/fairwindsk.desktop.in fairwindsk.desktop @ONLY)
        configure_file(cmake/fairwindsk-startup.desktop.in fairwindsk-startup.desktop @ONLY)
        configure_file(cmake/InstallRaspberryDesktopIntegration.cmake.in
                InstallRaspberryDesktopIntegration.cmake @ONLY)

        install(PROGRAMS extras/fairwindsk-startup
                DESTINATION ${CMAKE_INSTALL_BINDIR})
        install(FILES "${CMAKE_CURRENT_BINARY_DIR}/fairwindsk.desktop"
                DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
        install(FILES resources/images/mainwindow/fairwind_icon.png
                DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps
                RENAME fairwindsk.png)
        install(SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/InstallRaspberryDesktopIntegration.cmake")
    endif()
endif()

# Let Qt collect the platform plug-ins, libraries, and frameworks needed by a
# portable desktop installation. Run this after custom shared libraries have
# been installed so macdeployqt and windeployqt can resolve every dependency.
# Qt 6.4 exposes this command with an older positional signature, while Qt 6.5
# and newer accept OUTPUT_SCRIPT and can generate the portable package script.
if(NOT FAIRWINDSK_MOBILE AND Qt6_VERSION VERSION_GREATER_EQUAL "6.5"
        AND COMMAND qt_generate_deploy_app_script)
    qt_generate_deploy_app_script(
            TARGET ${PROJECT_NAME}
            OUTPUT_SCRIPT FAIRWINDSK_DEPLOY_SCRIPT
            NO_UNSUPPORTED_PLATFORM_ERROR
    )
    install(SCRIPT ${FAIRWINDSK_DEPLOY_SCRIPT})
endif()

# CPack provides consistent archive names while retaining native macOS DMG and
# Debian package formats where they improve the installation experience.
set(CPACK_PACKAGE_NAME "FairWindSK")
set(CPACK_PACKAGE_VENDOR "OpenFairWind")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Marine multi-functional display for Signal K")
set(CPACK_PACKAGE_VERSION "${FAIRWINDSK_SEMVER}")
set(CPACK_PACKAGE_FILE_NAME "FairWindSK-${FAIRWINDSK_SEMVER}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
if(WIN32)
    set(CPACK_GENERATOR "ZIP")
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
elseif(FAIRWINDSK_RASPBERRY_PI_OS)
    set(CPACK_GENERATOR "DEB")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "OpenFairWind")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "arm64")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
else()
    set(CPACK_GENERATOR "TGZ")
endif()
include(CPack)
