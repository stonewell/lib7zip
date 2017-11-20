cmake_minimum_required(VERSION 2.8)

project(lib7zip)

option(LIB7ZIP_BUILD_TESTS "Build tests for lib7zip" OFF)

set(PATCH_DIR ${CMAKE_CURRENT_SOURCE_DIR}/patches)

if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
set(UPSTREAM_SRC_URL http://www.7-zip.org/a/7z1604-src.7z)
set(UPSTREAM_PATCH_CMD dos2unix ${PATCH_DIR}/001_avoid_duplicate_guids.patch COMMAND patch --ignore-whitespace --strip 1 < ${PATCH_DIR}/001_avoid_duplicate_guids.patch COMMAND unix2dos ${CMAKE_CURRENT_BINARY_DIR}/upstream-7zip-src/CPP/Windows/FileIO.h)
else() # Windows
set(UPSTREAM_SRC_URL https://downloads.sourceforge.net/project/p7zip/p7zip/16.02/p7zip_16.02_src_all.tar.bz2)
set(UPSTREAM_PATCH_CMD )
endif() # not Windows

include(DownloadProject.cmake)
download_project(
  PROJ upstream-7zip
  URL ${UPSTREAM_SRC_URL}
  PATCH_COMMAND ${UPSTREAM_PATCH_CMD}
)

set(UPSTREAM_DIR ${PROJECT_BINARY_DIR}/upstream-7zip-src)
set(UPSTREAM_INCLUDE_DIR ${UPSTREAM_DIR})

add_subdirectory(Lib7Zip)

if (LIB7ZIP_BUILD_TESTS)
  add_subdirectory(Test7Zip)
endif()

