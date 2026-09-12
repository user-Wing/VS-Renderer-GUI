cmake_minimum_required(VERSION 3.25)

project(VSRenderer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Widgets)

qt_add_executable(VSRenderer WIN32
    src/main.cpp
    src/app/MainWindow.cpp
    src/app/MainWindow.h
    src/app/Style.cpp
    src/app/Style.h
    src/backend/ThreeFpApi.cpp
    src/backend/ThreeFpApi.h
    src/backend/ThreeFpPlayer.cpp
    src/backend/ThreeFpPlayer.h
    src/backend/FrameTimeline.h
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
    src/ui/ParameterEditor.cpp
    src/ui/ParameterEditor.h
    src/ui/PreviewPane.cpp
    src/ui/PreviewPane.h
)

target_include_directories(VSRenderer PRIVATE src third_party/vapoursynth/include)
target_link_libraries(VSRenderer PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)
target_compile_definitions(VSRenderer PRIVATE VSR_VERSION="${PROJECT_VERSION}")

if(WIN32)
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

include(CTest)
if(BUILD_TESTING)
    find_package(Qt6 6.8 REQUIRED COMPONENTS Test)
    qt_add_executable(vsr_graph_tests
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
        tests/TestFrameBridge.cpp
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
        src/ui/PreviewPane.cpp
        src/ui/PreviewPane.h
    )
    target_include_directories(vsr_preview_tests PRIVATE src)
    target_link_libraries(vsr_preview_tests PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Test)
    add_test(NAME preview-zoom-and-pan COMMAND vsr_preview_tests)

    if(WIN32)
        set_tests_properties(graph-and-vpy vapoursynth-to-3fp-frame-bridge preview-zoom-and-pan
            PROPERTIES ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    endif()
endif()
