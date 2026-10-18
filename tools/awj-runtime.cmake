# Keep the unmodified standalone CLI and matching source; GUI is not needed.
set(awj_directory "${CMAKE_SOURCE_DIR}/.deps/awj/1.2.1")
set(awj_archive "${awj_directory}/AWJ_Win.7z")
set(awj_source "${awj_directory}/AWJimage_1.2.1_Source.tar.gz")
file(MAKE_DIRECTORY "${awj_directory}")
if(NOT EXISTS "${awj_archive}")
    file(DOWNLOAD "https://github.com/Dominic485649/AWJimage/releases/download/1.2.1/AWJ_Win.7z"
        "${awj_archive}" EXPECTED_HASH SHA256=4b4431afc8f3b54a164a697fabe3589bf93bbb4388321a9459f0be0a40196fca TLS_VERIFY ON)
endif()
file(SHA256 "${awj_archive}" awj_hash)
if(NOT awj_hash STREQUAL "4b4431afc8f3b54a164a697fabe3589bf93bbb4388321a9459f0be0a40196fca")
    message(FATAL_ERROR "AWJimage archive hash mismatch")
endif()
file(ARCHIVE_EXTRACT INPUT "${awj_archive}" DESTINATION "${awj_directory}/bin")
if(NOT EXISTS "${awj_source}")
    file(DOWNLOAD "https://github.com/Dominic485649/AWJimage/releases/download/1.2.1/AWJimage_1.2.1_Source.tar.gz"
        "${awj_source}" EXPECTED_HASH SHA256=fb77311ea822ff4425c013dfc0e4400ea859e2ae5ffd1c0f4ca765472deeb66f TLS_VERIFY ON)
endif()
file(SHA256 "${awj_source}" awj_source_hash)
if(NOT awj_source_hash STREQUAL "fb77311ea822ff4425c013dfc0e4400ea859e2ae5ffd1c0f4ca765472deeb66f")
    message(FATAL_ERROR "AWJimage source archive hash mismatch")
endif()
add_custom_command(TARGET VSPlayer POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_FILE_DIR:VSPlayer>/runtime/awj-source"
    COMMAND "${CMAKE_COMMAND}" -E tar cf "$<TARGET_FILE_DIR:VSPlayer>/runtime/awj.zip" --format=zip AWJ.com LICENSE NOTICE.txt
    COMMAND "${CMAKE_COMMAND}" -E copy "${awj_source}" "$<TARGET_FILE_DIR:VSPlayer>/runtime/awj-source/"
    COMMAND "${CMAKE_COMMAND}" -E copy "${awj_directory}/bin/LICENSE" "${awj_directory}/bin/NOTICE.txt" "${CMAKE_SOURCE_DIR}/third_party/runtime-notices/awjimage-NOTICE.txt" "$<TARGET_FILE_DIR:VSPlayer>/runtime/awj-source/"
    WORKING_DIRECTORY "${awj_directory}/bin"
    VERBATIM)
