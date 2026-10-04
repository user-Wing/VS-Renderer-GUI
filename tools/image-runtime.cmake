# Decode through libavif directly: avifdec's CLI caps pixel count at 256 MP.
function(vsr_image_dependency name url digest)
    set(archive "${CMAKE_SOURCE_DIR}/.deps/image/${name}.archive")
    set(directory "${CMAKE_SOURCE_DIR}/.deps/image/${name}")
    if(NOT EXISTS "${directory}/.ready")
        file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/.deps/image")
        file(DOWNLOAD "${url}" "${archive}" EXPECTED_HASH "SHA256=${digest}" TLS_VERIFY ON)
        file(ARCHIVE_EXTRACT INPUT "${archive}" DESTINATION "${directory}")
        file(WRITE "${directory}/.ready" "${digest}")
    endif()
endfunction()
vsr_image_dependency(libdeflate "https://github.com/ebiggers/libdeflate/archive/refs/tags/v1.25.tar.gz"
    "d11473c1ad4c57d874695e8026865e38b47116bbcb872bfc622ec8f37a86017d")
set(LIBDEFLATE_BUILD_SHARED_LIB OFF CACHE BOOL "" FORCE)
set(LIBDEFLATE_BUILD_GZIP OFF CACHE BOOL "" FORCE)
set(LIBDEFLATE_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory("${CMAKE_SOURCE_DIR}/.deps/image/libdeflate/libdeflate-1.25" "${CMAKE_BINARY_DIR}/image-libdeflate" EXCLUDE_FROM_ALL)
vsr_image_dependency(libavif "https://github.com/AOMediaCodec/libavif/archive/refs/tags/v1.4.2.tar.gz"
    "2b645287340ba5a631d268b551dc2d72bd73ac33335962dd36dcdb6d8366921d")
vsr_image_dependency(dav1d "https://repo.msys2.org/mingw/mingw64/mingw-w64-x86_64-dav1d-1.5.3-1-any.pkg.tar.zst"
    "18e1372c2218acc5deb17b6154b3159f9f72eb0ff8051b2f798f9d1344b2cff4")
# Revision recommended by libavif 1.4.2; build SIMD conversion with our compiler.
vsr_image_dependency(libyuv "https://github.com/lemenkov/libyuv/archive/644251f25.tar.gz"
    "ccc11fbb02077b9385a37606d9edce34eb5e588b5626c894eb1e883c9c2da114")
# Upstream also caps the API at 256 MP. Permit an explicit larger limit;
# dimension checks, grid validation and dav1d bitstream checks remain active.
set(avif_reader "${CMAKE_SOURCE_DIR}/.deps/image/libavif/libavif-1.4.2/src/read.c")
file(READ "${avif_reader}" avif_reader_source)
set(original_limit "if ((decoder->imageSizeLimit > AVIF_DEFAULT_IMAGE_SIZE_LIMIT) || (decoder->imageSizeLimit == 0))")
set(large_limit "if (decoder->imageSizeLimit == 0)")
string(FIND "${avif_reader_source}" "${original_limit}" original_limit_at)
string(FIND "${avif_reader_source}" "${large_limit}" large_limit_at)
if(original_limit_at GREATER_EQUAL 0)
    string(REPLACE "${original_limit}" "${large_limit}" avif_reader_source "${avif_reader_source}")
    file(WRITE "${avif_reader}" "${avif_reader_source}")
elseif(large_limit_at LESS 0)
    message(FATAL_ERROR "libavif image size guard changed; review the large-image patch")
endif()
function(vsr_build_image_decoder)
    set(BUILD_SHARED_LIBS OFF)
    set(AVIF_BUILD_APPS OFF CACHE BOOL "" FORCE)
    set(AVIF_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    add_subdirectory("${CMAKE_SOURCE_DIR}/.deps/image/libyuv/libyuv-644251f252a84bf8ce91ff0aca86a9b16b069ab8" "${CMAKE_BINARY_DIR}/image-libyuv" EXCLUDE_FROM_ALL)
    add_library(yuv::yuv ALIAS yuv)
    target_include_directories(yuv INTERFACE "${CMAKE_SOURCE_DIR}/.deps/image/libyuv/libyuv-644251f252a84bf8ce91ff0aca86a9b16b069ab8/include")
    set(LIBYUV_VERSION 1924)
    set(AVIF_LIBYUV SYSTEM CACHE STRING "" FORCE)
    set(AVIF_CODEC_DAV1D SYSTEM CACHE STRING "" FORCE)
    set(DAV1D_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/.deps/image/dav1d/mingw64/include" CACHE PATH "" FORCE)
    set(DAV1D_LIBRARY "${CMAKE_SOURCE_DIR}/.deps/image/dav1d/mingw64/lib/libdav1d.a" CACHE FILEPATH "" FORCE)
    add_subdirectory("${CMAKE_SOURCE_DIR}/.deps/image/libavif/libavif-1.4.2" "${CMAKE_BINARY_DIR}/image-libavif" EXCLUDE_FROM_ALL)
endfunction()
vsr_build_image_decoder()

# Compile with the application's MinGW runtime; prebuilt archives use a newer
# printf/varargs ABI and can crash in codec diagnostics.
vsr_image_dependency(turbojpeg-source "https://github.com/libjpeg-turbo/libjpeg-turbo/archive/refs/tags/3.2.0.tar.gz"
    "980dd81f425082aa6d7c9e47fef27554ce7a9ffc8e2f6e863b97d263c5c50858")
vsr_image_dependency(nasm "https://www.nasm.us/pub/nasm/releasebuilds/2.16.03/win64/nasm-2.16.03-win64.zip"
    "3ee4782247bcb874378d02f7eab4e294a84d3d15f3f6ee2de2f47a46aa7226e6")
include(ExternalProject)
ExternalProject_Add(vsr_turbojpeg_build
    SOURCE_DIR "${CMAKE_SOURCE_DIR}/.deps/image/turbojpeg-source/libjpeg-turbo-3.2.0"
    BINARY_DIR "${CMAKE_BINARY_DIR}/image-turbojpeg"
    CMAKE_ARGS
        -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
        -DCMAKE_BUILD_TYPE=Release -DENABLE_SHARED=OFF -DENABLE_STATIC=ON
        -DWITH_TOOLS=OFF -DWITH_TESTS=OFF -DWITH_SIMD=ON -DWITH_JAVA=OFF
        -DCMAKE_ASM_NASM_COMPILER=${CMAKE_SOURCE_DIR}/.deps/image/nasm/nasm-2.16.03/nasm.exe
    BUILD_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --target turbojpeg-static --parallel 4
    INSTALL_COMMAND ""
    BUILD_BYPRODUCTS "${CMAKE_BINARY_DIR}/image-turbojpeg/libturbojpeg.a")
add_library(vsr_turbojpeg STATIC IMPORTED GLOBAL)
set_target_properties(vsr_turbojpeg PROPERTIES
    IMPORTED_LOCATION "${CMAKE_BINARY_DIR}/image-turbojpeg/libturbojpeg.a"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/.deps/image/turbojpeg-source/libjpeg-turbo-3.2.0/src")
add_dependencies(vsr_turbojpeg vsr_turbojpeg_build)
target_link_libraries(VSPlayer PRIVATE vsr_turbojpeg)
