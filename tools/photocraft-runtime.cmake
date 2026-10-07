# Rust is a build-time dependency only. Build the pinned component with build-photocraft.ps1.
set(VSR_PHOTOCRAFT_RUNTIME "${CMAKE_SOURCE_DIR}/build/photocraft-runtime")
if(EXISTS "${VSR_PHOTOCRAFT_RUNTIME}/photocraft.exe")
    add_custom_command(TARGET VSPlayer POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${VSR_PHOTOCRAFT_RUNTIME}" "$<TARGET_FILE_DIR:VSPlayer>/runtime/photocraft"
        VERBATIM)
endif()
