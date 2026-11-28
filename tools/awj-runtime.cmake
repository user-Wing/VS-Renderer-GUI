# Keep the unmodified standalone CLI and matching source; GUI is not needed.
set(awj_directory "${CMAKE_SOURCE_DIR}/.deps/awj/1.3.7")
set(awj_archive "${awj_directory}/AWJ_Win.7z")
set(awj_source "${awj_directory}/AWJimage-1.3.7-source.zip")
file(MAKE_DIRECTORY "${awj_directory}")
if(NOT EXISTS "${awj_archive}")
    file(DOWNLOAD "https://github.com/Dominic485649/AWJimage/releases/download/1.3.7/AWJ_Win.7z"
        "${awj_archive}" EXPECTED_HASH SHA256=f8ad1b6734e4242a3ad432847fecdb42adac5ec5c0f3fed1de95b5fbeb039a50 TLS_VERIFY ON)
endif()
file(SHA256 "${awj_archive}" awj_hash)
if(NOT awj_hash STREQUAL "f8ad1b6734e4242a3ad432847fecdb42adac5ec5c0f3fed1de95b5fbeb039a50")
    message(FATAL_ERROR "AWJimage archive hash mismatch")
endif()
file(ARCHIVE_EXTRACT INPUT "${awj_archive}" DESTINATION "${awj_directory}/bin")
if(NOT EXISTS "${awj_source}")
    file(DOWNLOAD "https://github.com/Dominic485649/AWJimage/releases/download/1.3.7/AWJimage-1.3.7-source.zip"
        "${awj_source}" EXPECTED_HASH SHA256=74a9f205197917b7a9ab4025a9f8b05808407257eb0783079636737ddb214270 TLS_VERIFY ON)
endif()
file(SHA256 "${awj_source}" awj_source_hash)
if(NOT awj_source_hash STREQUAL "74a9f205197917b7a9ab4025a9f8b05808407257eb0783079636737ddb214270")
    message(FATAL_ERROR "AWJimage source archive hash mismatch")
endif()
add_custom_command(TARGET VSPlayer POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_FILE_DIR:VSPlayer>/licenses/AWJ" "$<TARGET_FILE_DIR:VSPlayer>/tools/awj-s"
    COMMAND "${CMAKE_COMMAND}" -E tar cf "$<TARGET_FILE_DIR:VSPlayer>/tools/awj-s/awj.7z" --format=7zip AWJ.com
    COMMAND "${CMAKE_COMMAND}" -E rm -f "$<TARGET_FILE_DIR:VSPlayer>/tools/awj-s/awj.zip"
    COMMAND "${CMAKE_COMMAND}" -E copy "${awj_source}" "$<TARGET_FILE_DIR:VSPlayer>/licenses/AWJ/"
    COMMAND "${CMAKE_COMMAND}" -E copy "${awj_directory}/bin/LICENSE" "${awj_directory}/bin/NOTICE.txt" "${CMAKE_SOURCE_DIR}/third_party/runtime-notices/awjimage-NOTICE.txt" "$<TARGET_FILE_DIR:VSPlayer>/licenses/AWJ/"
    WORKING_DIRECTORY "${awj_directory}/bin"
    VERBATIM)
