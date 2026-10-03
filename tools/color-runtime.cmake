set(VSR_COLOR_ROOT "${CMAKE_SOURCE_DIR}/.deps/color" CACHE PATH "Optional D3D11 color engine SDK")
if(WIN32 AND EXISTS "${VSR_COLOR_ROOT}/ucrt64/include/libplacebo/config.h")
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_library(vsr_color SHARED src/color/ColorBridge.c src/color/ColorBridge.h)
    set_target_properties(vsr_color PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF
        PREFIX "" OUTPUT_NAME "vsr-color" C_STANDARD 17)
    target_include_directories(vsr_color PRIVATE "${VSR_COLOR_ROOT}/ucrt64/include"
        "${CMAKE_SOURCE_DIR}/.deps/fff-player/third_party/ffmpeg/include")
    target_link_libraries(vsr_color PRIVATE "${VSR_COLOR_ROOT}/ucrt64/lib/libplacebo.dll.a"
        "${CMAKE_SOURCE_DIR}/.deps/fff-player/third_party/ffmpeg/lib/x64/avutil-61.lib"
        d3d11 dxgi dxguid gdi32)
    add_custom_command(TARGET vsr_color POST_BUILD
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/stage-color.py"
            --dependencies "${VSR_COLOR_ROOT}" --bridge "$<TARGET_FILE:vsr_color>"
            --output "${CMAKE_BINARY_DIR}/color"
        VERBATIM)
    if(BUILD_TESTING)
        add_executable(vsr_color_metadata_tests tests/TestColorMetadata.c)
        set_target_properties(vsr_color_metadata_tests PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF C_STANDARD 17)
        target_include_directories(vsr_color_metadata_tests PRIVATE src "${CMAKE_SOURCE_DIR}/.deps/fff-player/third_party/ffmpeg/include")
        target_link_libraries(vsr_color_metadata_tests PRIVATE d3d11
            "${CMAKE_SOURCE_DIR}/.deps/fff-player/third_party/ffmpeg/lib/x64/avutil-61.lib")
        add_dependencies(vsr_color_metadata_tests vsr_color)
        add_test(NAME color-hdr-metadata COMMAND vsr_color_metadata_tests)
    endif()
endif()
