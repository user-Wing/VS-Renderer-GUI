cmake_minimum_required(VERSION 3.25)

project(VSRenderer VERSION 1.0.5 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Widgets Network Concurrent)
qt_add_library(vsr_bluray STATIC src/bluray/BlurayCatalog.cpp src/bluray/BlurayCatalog.h
    src/bluray/BlurayWidget.cpp src/bluray/BlurayWidget.h)
target_include_directories(vsr_bluray PUBLIC src)
target_link_libraries(vsr_bluray PUBLIC Qt6::Widgets Qt6::Concurrent)
qt_add_library(vsr_components STATIC src/update/ComponentDownloads.cpp src/update/ComponentDownloads.h src/update/PortableUpdater.cpp src/update/PortableUpdater.h)
target_include_directories(vsr_components PUBLIC src)
target_link_libraries(vsr_components PUBLIC Qt6::Widgets Qt6::Network Qt6::Concurrent)
target_compile_definitions(vsr_components PRIVATE VSR_VERSION="${PROJECT_VERSION}")
if(WIN32)
    target_link_libraries(vsr_components PUBLIC version)
endif()
include(tools/color-runtime.cmake)
add_library(vsr_image_zlib STATIC
    third_party/image-zlib/adler32.c third_party/image-zlib/crc32.c
    third_party/image-zlib/inflate.c third_party/image-zlib/inffast.c
    third_party/image-zlib/inftrees.c third_party/image-zlib/zutil.c
    third_party/image-zlib/deflate.c third_party/image-zlib/trees.c)
target_include_directories(vsr_image_zlib PUBLIC third_party/image-zlib)
qt_add_library(vsr_image_core STATIC src/image/ImageDocument.cpp src/image/ImageDocument.h
    src/image/ImagePsd.cpp src/image/ImagePsd.h)
target_include_directories(vsr_image_core PUBLIC src)
target_link_libraries(vsr_image_core PUBLIC Qt6::Core Qt6::Gui Qt6::Widgets PRIVATE vsr_image_zlib)
qt_add_library(vsr_image_editor STATIC
    src/image/ImageEditorTools.cpp src/image/ImageEditorTools.h
    src/image/ImageEditorCanvas.cpp src/image/ImageEditorCanvas.h
    src/image/ImageAdjustments.cpp src/image/ImageAdjustments.h
    src/image/ImageHdr.cpp src/image/ImageHdr.h
    src/image/ImageHdrSurface.cpp src/image/ImageHdrSurface.h
    src/image/ImageEditorWindow.cpp src/image/ImageEditorWindow.h)
target_link_libraries(vsr_image_editor PUBLIC vsr_image_core)
if(WIN32)
    target_link_libraries(vsr_image_editor PRIVATE d3d11 dxgi d3dcompiler)
endif()
if(MINGW)
    # These icon-only resources need no C++ include paths or definitions.
    set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> -O coff <SOURCE> <OBJECT>")
endif()

qt_add_executable(VSRenderer WIN32
    assets/icons/icons.qrc
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
target_link_libraries(VSRenderer PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network vsr_bluray)
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
    assets/startup.qrc
    src/player/PlayerMenu.cpp src/player/PlayerMenu.h
    src/player/PlayerImageTools.cpp src/player/PlayerImageTools.h
    src/player/PlayerImageResample.cpp assets/image-tools.qrc
    src/player/main.cpp src/player/PlayerWindow.cpp src/player/PlayerControls.cpp src/player/PlayerInfo.cpp
    src/player/PlayerWindow.h
    src/player/PlayerSettings.cpp src/player/PlayerColor.cpp src/player/PlayerMenus.cpp src/player/PlayerPlaylist.cpp src/player/PlayerAudio.cpp src/player/PlayerMediaMatching.cpp
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
foreach(program VSRenderer VSPlayer)
    target_link_libraries(${program} PRIVATE vsr_components)
    add_custom_command(TARGET ${program} POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:Qt6::Concurrent>" "$<TARGET_FILE_DIR:${program}>"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_FILE_DIR:${program}>/tools"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${CMAKE_SOURCE_DIR}/tools/download-bd.py" "$<TARGET_FILE_DIR:${program}>/tools"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_FILE_DIR:${program}>/runtime/tools"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${CMAKE_SOURCE_DIR}/tools/apply-components.ps1" "$<TARGET_FILE_DIR:${program}>/runtime/tools"
        VERBATIM)
endforeach()
if(TARGET vsr_color)
    add_dependencies(VSRenderer vsr_color)
    add_dependencies(VSPlayer vsr_color)
endif()
if(WIN32)
    configure_file(assets/icons/player.rc "${CMAKE_CURRENT_BINARY_DIR}/player-icons.rc" @ONLY)
    set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/player-icons.rc" PROPERTIES OBJECT_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/player.ico;${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/video.ico;${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/image.ico")
    target_sources(VSPlayer PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/player-icons.rc")
endif()
include(tools/image-runtime.cmake)
qt_add_library(vsr_image_decoder STATIC src/player/PlayerImage.cpp src/player/PlayerImage.h src/player/PlayerPng.cpp src/player/PlayerPng.h)
target_include_directories(vsr_image_decoder PUBLIC src)
target_link_libraries(vsr_image_decoder PUBLIC Qt6::Core Qt6::Gui PRIVATE avif vsr_turbojpeg vsr_image_core libdeflate_static)
target_link_libraries(vsr_image_editor PRIVATE vsr_image_decoder)
include(tools/awj-runtime.cmake)
target_link_libraries(VSPlayer PRIVATE avif vsr_image_editor vsr_bluray)

target_include_directories(VSPlayer PRIVATE src third_party/vapoursynth/include)
target_link_libraries(VSPlayer PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network ole32 oleaut32 strmiids uuid dxgi psapi pdh gdi32 shell32)
target_compile_definitions(VSPlayer PRIVATE VSR_VERSION="${PROJECT_VERSION}")

if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/shaders/mpv-shaders.7z")
    foreach(program VSRenderer VSPlayer)
        add_custom_command(TARGET ${program} POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_directory
                "${CMAKE_SOURCE_DIR}/assets/mpv-shaders" "$<TARGET_FILE_DIR:${program}>/shaders/mpv-shaders"
            VERBATIM)
    endforeach()
endif()

include(CTest)
if(BUILD_TESTING)
    find_package(Qt6 6.8 REQUIRED COMPONENTS Test Network)
    qt_add_executable(vsr_bluray_tests tests/TestBluray.cpp)
    set_target_properties(vsr_bluray_tests PROPERTIES OUTPUT_NAME "vsr-bluray-tests")
    target_link_libraries(vsr_bluray_tests PRIVATE vsr_bluray Qt6::Test)
    add_test(NAME bluray-metadata COMMAND vsr_bluray_tests)
    qt_add_executable(vsr_image_tests tests/TestImageDocument.cpp)
    target_link_libraries(vsr_image_tests PRIVATE vsr_image_core Qt6::Test)
    add_test(NAME image-document-core COMMAND vsr_image_tests)
    qt_add_executable(vsr_image_layer_tests tests/TestImageLayerOperations.cpp)
    target_link_libraries(vsr_image_layer_tests PRIVATE vsr_image_core Qt6::Test)
    add_test(NAME image-layer-operations COMMAND vsr_image_layer_tests)
    qt_add_executable(vsr_image_psd_tests tests/TestImagePsd.cpp)
    target_link_libraries(vsr_image_psd_tests PRIVATE vsr_image_core Qt6::Test)
    add_test(NAME image-psd-layers COMMAND vsr_image_psd_tests)
    qt_add_executable(vsr_image_adjustments_tests tests/TestImageAdjustments.cpp)
    target_link_libraries(vsr_image_adjustments_tests PRIVATE vsr_image_editor Qt6::Test)
    add_test(NAME image-adjustments-hdr COMMAND vsr_image_adjustments_tests)
    qt_add_executable(vsr_image_editor_advanced_tests tests/TestImageEditorAdvanced.cpp)
    target_link_libraries(vsr_image_editor_advanced_tests PRIVATE vsr_image_editor Qt6::Test)
    add_test(NAME image-editor-advanced-tools COMMAND vsr_image_editor_advanced_tests)
    qt_add_executable(vsr_image_editor_layout_tests tests/TestImageEditorLayout.cpp)
    target_link_libraries(vsr_image_editor_layout_tests PRIVATE vsr_image_editor Qt6::Test)
    add_test(NAME image-editor-layout COMMAND vsr_image_editor_layout_tests)
    qt_add_executable(vsr_image_editor_interaction_tests tests/TestImageEditorInteraction.cpp)
    target_link_libraries(vsr_image_editor_interaction_tests PRIVATE vsr_image_editor Qt6::Test)
    add_test(NAME image-editor-interaction COMMAND vsr_image_editor_interaction_tests)
    qt_add_executable(vsr_image_editor_tests tests/TestImageEditor.cpp
        src/player/PlayerImageTools.cpp src/player/PlayerImageTools.h src/player/PlayerImageResample.cpp
        src/ui/PreviewPane.cpp src/ui/PreviewPane.h assets/image-tools.qrc)
    target_link_libraries(vsr_image_editor_tests PRIVATE vsr_image_editor Qt6::Test shell32)
    add_test(NAME image-editor-tools COMMAND vsr_image_editor_tests)
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

    qt_add_executable(vsr_parameter_tests assets/startup.qrc tests/TestParameterEditor.cpp src/ui/ParameterEditor.cpp src/ui/ParameterEditor.h src/graph/FilterCatalog.cpp src/graph/FilterGraph.cpp src/app/Style.cpp)
    target_include_directories(vsr_parameter_tests PRIVATE src)
    target_link_libraries(vsr_parameter_tests PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Test)
    add_test(NAME shader-parameters COMMAND vsr_parameter_tests)

    qt_add_executable(vsr_color_tests tests/TestColor.cpp assets/startup.qrc src/backend/ThreeFpApi.cpp
        src/backend/ThreeFpPlayer.cpp src/backend/VapourSynthFrameServer.cpp)
    target_include_directories(vsr_color_tests PRIVATE src third_party/vapoursynth/include)
    target_link_libraries(vsr_color_tests PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Test)
    if(TARGET vsr_color)
        add_dependencies(vsr_color_tests vsr_color)
    endif()
    add_test(NAME d3d11-color-management COMMAND vsr_color_tests -o color-test-results.txt,txt)

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
    target_link_libraries(vsr_analysis_tests PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Test Qt6::Network vsr_bluray)
    target_compile_definitions(vsr_analysis_tests PRIVATE VSR_VERSION="${PROJECT_VERSION}")
    add_test(NAME direct-video-comparison COMMAND vsr_analysis_tests -o analysis-test-results.txt,txt)

    get_target_property(player_test_sources VSPlayer SOURCES)
    list(FILTER player_test_sources EXCLUDE REGEX "src/player/main.cpp|VSPlayer_autogen")
    qt_add_executable(vsr_player_tests tests/TestPlayer.cpp ${player_test_sources})
    add_dependencies(vsr_player_tests VSPlayer)
    target_include_directories(vsr_player_tests PRIVATE src third_party/vapoursynth/include)
    target_link_libraries(vsr_player_tests PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Test Qt6::Network ole32 oleaut32 strmiids uuid dxgi psapi pdh gdi32 shell32)
    target_link_libraries(vsr_player_tests PRIVATE avif vsr_turbojpeg vsr_image_editor vsr_bluray)
    target_link_libraries(vsr_player_tests PRIVATE vsr_components)
    target_compile_definitions(vsr_player_tests PRIVATE VSR_VERSION="${PROJECT_VERSION}")
    add_test(NAME vs-player-playback COMMAND vsr_player_tests -o player-test-results.txt,txt)
    add_test(NAME player-language-packs COMMAND "${CMAKE_BINARY_DIR}/runtime/python/python.exe" "${CMAKE_SOURCE_DIR}/tools/check-player-languages.py")

    qt_add_executable(vsr_update_tests tests/TestUpdates.cpp)
    target_include_directories(vsr_update_tests PRIVATE src)
    target_link_libraries(vsr_update_tests PRIVATE Qt6::Core Qt6::Widgets Qt6::Network Qt6::Test)
    target_link_libraries(vsr_update_tests PRIVATE vsr_components)
    target_compile_definitions(vsr_update_tests PRIVATE VSR_VERSION="${PROJECT_VERSION}")
    add_test(NAME portable-full-update COMMAND vsr_update_tests -o update-test-results.txt,txt)

    if(WIN32)
        set_tests_properties(portable-full-update vs-player-playback direct-video-comparison graph-and-vpy vapoursynth-to-3fp-frame-bridge preview-zoom-and-pan export-command-compatibility
            PROPERTIES ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    endif()
endif()
