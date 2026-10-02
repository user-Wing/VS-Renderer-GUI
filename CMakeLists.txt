cmake_minimum_required(VERSION 3.25)

project(VSRenderer VERSION 1.0.3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Widgets Network)
if(MINGW)
    # These icon-only resources need no C++ include paths or definitions.
    set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> -O coff <SOURCE> <OBJECT>")
endif()

qt_add_executable(VSRenderer WIN32
    assets/icons/icons.qrc
    src/update/PortableUpdater.cpp src/update/PortableUpdater.h
    assets/startup.qrc
    src/backend/StartupWarmup.cpp
    src/backend/StartupWarmup.h
    src/ui/MultiCompareView.cpp
    src/ui/MultiCompareView.h
    src/ui/AnalysisExport.cpp
    src/ui/AnalysisPage.cpp
    src/ui/AnalysisPage.h
    src/graph/PresetStore.cpp
    src/ui/PresetDialog.cpp
    src/main.cpp
    src/app/MainWindowPresets.cpp
    src/app/MainWindow.cpp
    src/app/MainWindow.h
    src/app/Style.cpp
    src/app/Style.h
    src/backend/ThreeFpApi.cpp
    src/backend/ThreeFpApi.h
    src/backend/ThreeFpPlayer.cpp
    src/backend/ThreeFpPlayer.h
    src/backend/FrameTimeline.h
    src/backend/ExportPipeline.cpp
    src/backend/ExportPipeline.h
    src/backend/VapourSynthFrameServer.cpp
    src/backend/VapourSynthFrameServer.h
    src/graph/FilterCatalog.cpp
    src/graph/FilterCatalog.h
    src/graph/FilterGraph.cpp
    src/graph/FilterGraph.h
    src/graph/VpyScriptBuilder.cpp
    src/graph/VpyScriptBuilder.h
    src/ui/CollapsibleSection.cpp
    src/ui/CollapsibleSection.h
    src/ui/PreparedFiles.cpp
    src/ui/PreparedFiles.h
    src/ui/ExportWindow.cpp
    src/ui/ExportWindow.h
    src/ui/CompareView.cpp
    src/ui/CompareView.h
    src/ui/ParameterEditor.cpp
    src/ui/ParameterEditor.h
    src/ui/PreviewPane.cpp
    src/ui/PreviewPane.h
)

target_include_directories(VSRenderer PRIVATE src third_party/vapoursynth/include)
target_link_libraries(VSRenderer PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network)
target_compile_definitions(VSRenderer PRIVATE VSR_VERSION="${PROJECT_VERSION}")

if(WIN32)
    configure_file(assets/icons/renderer.rc "${CMAKE_CURRENT_BINARY_DIR}/renderer-icons.rc" @ONLY)
    set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/renderer-icons.rc" PROPERTIES OBJECT_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/renderer.ico")
    target_sources(VSRenderer PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/renderer-icons.rc")
    add_custom_command(TARGET VSRenderer POST_BUILD
        COMMAND powershell -NoProfile -ExecutionPolicy Bypass
            -File "${CMAKE_SOURCE_DIR}/tools/stage-vapoursynth-runtime.ps1"
            -OutputDirectory "$<TARGET_FILE_DIR:VSRenderer>/runtime/python"
        COMMENT "Staging bundled Python and VapourSynth runtime"
        VERBATIM
    )
endif()

if(MSVC)
    target_compile_options(VSRenderer PRIVATE /W4 /permissive-)
else()
    target_compile_options(VSRenderer PRIVATE -Wall -Wextra -Wpedantic)
endif()

qt_add_executable(VSPlayer WIN32
    assets/icons/icons.qrc
    src/update/PortableUpdater.cpp src/update/PortableUpdater.h
    assets/startup.qrc
    src/player/PlayerImage.cpp src/player/PlayerImage.h src/player/PlayerMenu.cpp src/player/PlayerMenu.h
    src/player/main.cpp src/player/PlayerWindow.cpp src/player/PlayerControls.cpp src/player/PlayerInfo.cpp
    src/player/PlayerWindow.h
    src/player/PlayerSettings.cpp src/player/PlayerMenus.cpp src/player/PlayerPlaylist.cpp src/player/PlayerAudio.cpp src/player/PlayerMediaMatching.cpp
    src/player/PlayerNetworkInput.cpp src/player/PlayerNetworkInput.h
    src/player/PlayerProfiles.cpp src/player/PlayerCache.cpp src/player/PlayerLanguage.cpp src/player/PlayerAssociations.cpp
    src/player/PlayerResources.cpp src/player/PlayerResources.h
    src/player/PlayerSubtitles.cpp src/player/PlayerSubtitles.h
    src/backend/LavPlayback.cpp src/backend/LavPlayback.h
    src/backend/ThreeFpApi.cpp src/backend/ThreeFpPlayer.cpp src/backend/VapourSynthFrameServer.cpp
    src/graph/FilterCatalog.cpp src/graph/FilterGraph.cpp src/graph/VpyScriptBuilder.cpp src/graph/PresetStore.cpp
    src/ui/PreviewPane.cpp
)
set_target_properties(VSPlayer PROPERTIES OUTPUT_NAME "vs-player")
if(WIN32)
    configure_file(assets/icons/player.rc "${CMAKE_CURRENT_BINARY_DIR}/player-icons.rc" @ONLY)
    set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/player-icons.rc" PROPERTIES OBJECT_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/player.ico;${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/video.ico;${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/image.ico")
    target_sources(VSPlayer PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/player-icons.rc")
endif()
include(tools/image-runtime.cmake)
target_link_libraries(VSPlayer PRIVATE avif)

target_include_directories(VSPlayer PRIVATE src third_party/vapoursynth/include)
target_link_libraries(VSPlayer PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network ole32 oleaut32 strmiids uuid dxgi psapi pdh gdi32 shell32)
target_compile_definitions(VSPlayer PRIVATE VSR_VERSION="${PROJECT_VERSION}")

foreach(program VSRenderer VSPlayer)
    add_custom_command(TARGET ${program} POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${CMAKE_SOURCE_DIR}/assets/mpv-shaders" "$<TARGET_FILE_DIR:${program}>/shaders/mpv-shaders"
        VERBATIM)
endforeach()

include(CTest)
if(BUILD_TESTING)
    find_package(Qt6 6.8 REQUIRED COMPONENTS Test Network)
    qt_add_executable(vsr_graph_tests
        assets/startup.qrc
        tests/TestVpyScript.cpp
        src/graph/FilterCatalog.cpp
        src/graph/FilterCatalog.h
        src/graph/FilterGraph.cpp
        src/graph/FilterGraph.h
        src/graph/VpyScriptBuilder.cpp
        src/graph/VpyScriptBuilder.h
    )
    target_include_directories(vsr_graph_tests PRIVATE src)
    target_link_libraries(vsr_graph_tests PRIVATE Qt6::Core Qt6::Test)
    add_test(NAME graph-and-vpy COMMAND vsr_graph_tests)

    qt_add_executable(vsr_frame_bridge_tests
        assets/startup.qrc
        src/backend/StartupWarmup.cpp
        src/backend/StartupWarmup.h
        tests/TestFrameBridge.cpp
        src/ui/CompareView.cpp
        src/ui/CompareView.h
        src/backend/ThreeFpApi.cpp
        src/backend/ThreeFpApi.h
        src/backend/ThreeFpPlayer.cpp
        src/backend/ThreeFpPlayer.h
        src/backend/VapourSynthFrameServer.cpp
        src/backend/VapourSynthFrameServer.h
        src/ui/PreviewPane.cpp
        src/ui/PreviewPane.h
    )
    target_include_directories(vsr_frame_bridge_tests PRIVATE src third_party/vapoursynth/include)
    target_link_libraries(vsr_frame_bridge_tests PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Test)
    add_test(NAME vapoursynth-to-3fp-frame-bridge COMMAND vsr_frame_bridge_tests)

    qt_add_executable(vsr_preview_tests
        tests/TestPreviewPane.cpp
        src/ui/CompareView.cpp
        src/ui/CompareView.h
        src/ui/PreviewPane.cpp
        src/ui/PreviewPane.h
    )
    target_include_directories(vsr_preview_tests PRIVATE src)
    target_link_libraries(vsr_preview_tests PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Test)
    add_test(NAME preview-zoom-and-pan COMMAND vsr_preview_tests)

    qt_add_executable(vsr_export_tests
        tests/TestExportPipeline.cpp
        src/app/Style.cpp
        src/app/Style.h
        src/ui/PreparedFiles.cpp
        src/ui/PreparedFiles.h
        src/ui/ExportWindow.cpp
        src/ui/ExportWindow.h
        src/backend/ExportPipeline.cpp
        src/backend/ExportPipeline.h
    )
    target_include_directories(vsr_export_tests PRIVATE src)
    target_link_libraries(vsr_export_tests PRIVATE Qt6::Core Qt6::Widgets Qt6::Test)
    add_test(NAME export-command-compatibility COMMAND vsr_export_tests)

    get_target_property(analysis_test_sources VSRenderer SOURCES)
    list(FILTER analysis_test_sources EXCLUDE REGEX "src/main.cpp|VSRenderer_autogen")
    qt_add_executable(vsr_analysis_tests tests/TestAnalysis.cpp ${analysis_test_sources})
    target_include_directories(vsr_analysis_tests PRIVATE src third_party/vapoursynth/include)
    target_link_libraries(vsr_analysis_tests PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Test Qt6::Network)
    target_compile_definitions(vsr_analysis_tests PRIVATE VSR_VERSION="${PROJECT_VERSION}")
    add_test(NAME direct-video-comparison COMMAND vsr_analysis_tests -o analysis-test-results.txt,txt)

    get_target_property(player_test_sources VSPlayer SOURCES)
    list(FILTER player_test_sources EXCLUDE REGEX "src/player/main.cpp|VSPlayer_autogen")
    qt_add_executable(vsr_player_tests tests/TestPlayer.cpp ${player_test_sources})
    target_include_directories(vsr_player_tests PRIVATE src third_party/vapoursynth/include)
    target_link_libraries(vsr_player_tests PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Test Qt6::Network ole32 oleaut32 strmiids uuid dxgi psapi pdh gdi32 shell32)
    target_link_libraries(vsr_player_tests PRIVATE avif vsr_turbojpeg)
    target_compile_definitions(vsr_player_tests PRIVATE VSR_VERSION="${PROJECT_VERSION}")
    add_test(NAME vs-player-playback COMMAND vsr_player_tests -o player-test-results.txt,txt)
    add_test(NAME player-language-packs COMMAND "${CMAKE_BINARY_DIR}/runtime/python/python.exe" "${CMAKE_SOURCE_DIR}/tools/check-player-languages.py")

    qt_add_executable(vsr_update_tests tests/TestUpdates.cpp src/update/PortableUpdater.cpp src/update/PortableUpdater.h)
    target_include_directories(vsr_update_tests PRIVATE src)
    target_link_libraries(vsr_update_tests PRIVATE Qt6::Core Qt6::Widgets Qt6::Network Qt6::Test)
    target_compile_definitions(vsr_update_tests PRIVATE VSR_VERSION="${PROJECT_VERSION}")
    add_test(NAME portable-full-update COMMAND vsr_update_tests -o update-test-results.txt,txt)

    if(WIN32)
        set_tests_properties(portable-full-update vs-player-playback direct-video-comparison graph-and-vpy vapoursynth-to-3fp-frame-bridge preview-zoom-and-pan export-command-compatibility
            PROPERTIES ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    endif()
endif()
