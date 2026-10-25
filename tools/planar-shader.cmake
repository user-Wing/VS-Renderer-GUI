set(VSR_VULKAN_HEADERS "${CMAKE_SOURCE_DIR}/.deps/vulkan-headers/include" CACHE PATH "Khronos Vulkan headers")
if(WIN32 AND TARGET vsr_color AND EXISTS "${VSR_VULKAN_HEADERS}/vulkan/vulkan.h")
    add_library(vsr_planar_shader SHARED
        third_party/vsr-placebo/init.c third_party/vsr-placebo/shader.c)
    set_target_properties(vsr_planar_shader PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF
        PREFIX "" OUTPUT_NAME "vsrplacebo" C_STANDARD 17)
    target_include_directories(vsr_planar_shader PRIVATE third_party/vapoursynth/include
        "${VSR_COLOR_ROOT}/ucrt64/include" "${VSR_VULKAN_HEADERS}")
    target_link_libraries(vsr_planar_shader PRIVATE "${VSR_COLOR_ROOT}/ucrt64/lib/libplacebo.dll.a")
    add_dependencies(vsr_planar_shader vsr_color)
    add_custom_command(TARGET vsr_planar_shader POST_BUILD
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/stage-planar-shader.py"
            --source "$<TARGET_FILE:vsr_planar_shader>"
            --output "${CMAKE_BINARY_DIR}/runtime/python/Lib/site-packages/vapoursynth/plugins/vsrplacebo.dll"
            --color "${CMAKE_BINARY_DIR}/color" VERBATIM)
endif()
