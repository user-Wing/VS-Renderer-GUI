# Keep the unmodified standalone CLI and matching source; GUI is not needed.
set(awj_directory "${CMAKE_SOURCE_DIR}/.deps/awj/1.3.1")
set(awj_archive "${awj_directory}/AWJ_Win.7z")
set(awj_source "${awj_directory}/AWJimage-1.3.1-source.zip")
file(MAKE_DIRECTORY "${awj_directory}")
if(NOT EXISTS "${awj_archive}")
    file(DOWNLOAD "https://github.com/Dominic485649/AWJimage/releases/download/1.3.1/AWJ_Win.7z"
        "${awj_archive}" EXPECTED_HASH SHA256=5b4d2a16f7d6f7c74c84b23f88c088b77a9244671316a31dcd7826da8a74f865 TLS_VERIFY ON)
endif()
file(SHA256 "${awj_archive}" awj_hash)
if(NOT awj_hash STREQUAL "5b4d2a16f7d6f7c74c84b23f88c088b77a9244671316a31dcd7826da8a74f865")
    message(FATAL_ERROR "AWJimage archive hash mismatch")
endif()
file(ARCHIVE_EXTRACT INPUT "${awj_archive}" DESTINATION "${awj_directory}/bin")
if(NOT EXISTS "${awj_source}")
    file(DOWNLOAD "https://github.com/Dominic485649/AWJimage/releases/download/1.3.1/AWJimage-1.3.1-source.zip"
        "${awj_source}" EXPECTED_HASH SHA256=82a33e3f54acaca830c37d03d3c671a0728473250e21b529030c44b4e4697124 TLS_VERIFY ON)
endif()
file(SHA256 "${awj_source}" awj_source_hash)
if(NOT awj_source_hash STREQUAL "82a33e3f54acaca830c37d03d3c671a0728473250e21b529030c44b4e4697124")
    message(FATAL_ERROR "AWJimage source archive hash mismatch")
endif()
add_custom_command(TARGET VSPlayer POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_FILE_DIR:VSPlayer>/runtime/awj-source"
    COMMAND "${CMAKE_COMMAND}" -E tar cf "$<TARGET_FILE_DIR:VSPlayer>/runtime/awj.7z" --format=7zip AWJ.com LICENSE NOTICE.txt
    COMMAND "${CMAKE_COMMAND}" -E rm -f "$<TARGET_FILE_DIR:VSPlayer>/runtime/awj.zip"
    COMMAND "${CMAKE_COMMAND}" -E copy "${awj_source}" "$<TARGET_FILE_DIR:VSPlayer>/runtime/awj-source/"
    COMMAND "${CMAKE_COMMAND}" -E copy "${awj_directory}/bin/LICENSE" "${awj_directory}/bin/NOTICE.txt" "${CMAKE_SOURCE_DIR}/third_party/runtime-notices/awjimage-NOTICE.txt" "$<TARGET_FILE_DIR:VSPlayer>/runtime/awj-source/"
    WORKING_DIRECTORY "${awj_directory}/bin"
    VERBATIM)
