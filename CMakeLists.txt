CMAKE_MINIMUM_REQUIRED(VERSION 3.12)
include(ExternalProject)

project(lib7zip)

option(LIB7ZIP_BUILD_TESTS "Build tests for lib7zip" OFF)

set(PATCH_DIR ${CMAKE_CURRENT_SOURCE_DIR}/patches)

set(UPSTREAM_DIR_NAME "lzma-sdk")

set(UPSTREAM_DIR ${CMAKE_BINARY_DIR}/${UPSTREAM_DIR_NAME})
set(UPSTREAM_INCLUDE_DIR ${UPSTREAM_DIR})
get_filename_component(UPSTREAM_DIR ${UPSTREAM_DIR} ABSOLUTE)

set(UPSTREAM_SRC_URL https://www.7-zip.org/a/lzma1805.7z)
if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set(PATCH_PATH "${PATCH_DIR}/001_avoid_duplicate_guids.patch")
    #set(UPSTREAM_PATCH_CMD "git apply --verbose --no-index ${PATCH_PATH}")
    #overcoming bug in CMake
    set(PATCH_SCRIPT "${CMAKE_BINARY_DIR}/patchProto.cmd")
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/patchProto.cmd.in" "${PATCH_SCRIPT}")
    set(UPSTREAM_PATCH_CMD "${PATCH_SCRIPT}")
else() # Windows
    set(UPSTREAM_PATCH_CMD )
endif() # not Windows

ExternalProject_Add(
	upstream-7zip
	URL ${UPSTREAM_SRC_URL}
	SOURCE_DIR ${UPSTREAM_DIR}
	PATCH_COMMAND ${UPSTREAM_PATCH_CMD}
    UPDATE_COMMAND ""
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
)

add_subdirectory(Lib7Zip)

if (LIB7ZIP_BUILD_TESTS)
  add_subdirectory(Test7Zip)
endif()

