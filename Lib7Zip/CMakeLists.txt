cmake_minimum_required(VERSION 2.8)

project(lib7zip)

set (SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# ==> 7-zip source download start

set(UPSTREAM_SRC_URL http://www.7-zip.org/a/7z1604-src.7z)
set(UPSTREAM_DIR ${PROJECT_BINARY_DIR}/upstream-7zip)
include(ExternalProject)
ExternalProject_Add(
  upstream
  URL ${UPSTREAM_SRC_URL}
  SOURCE_DIR ${UPSTREAM_DIR}
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
  PATCH_COMMAND dos2unix ${CMAKE_CURRENT_SOURCE_DIR}/001_avoid_duplicate_guids.patch COMMAND patch --ignore-whitespace --strip 1 < ${CMAKE_CURRENT_SOURCE_DIR}/001_avoid_duplicate_guids.patch COMMAND unix2dos ${UPSTREAM_DIR}/CPP/Windows/FileIO.h
)

set(UPSTREAM_INCLUDE_DIR ${UPSTREAM_DIR})

# <== 7-zip source download end

include_directories(
  ${UPSTREAM_INCLUDE_DIR}
  ${UPSTREAM_INCLUDE_DIR}/CPP
)

# TODO: includes

set (lib7zip_SOURCES
  7ZipArchive.cpp
  7ZipArchiveItem.cpp
  7zipLibrary.cpp
  HelperFuncs.cpp
  7ZipArchiveOpenCallback.cpp
  7ZipCodecInfo.cpp
  7ZipCompressCodecsInfo.cpp
  7ZipDllHandler.cpp
  7ZipFormatInfo.cpp
  7ZipObjectPtrArray.cpp
  7ZipOpenArchive.cpp
  OSFunctions_UnixLike.cpp
  OSFunctions_Win32.cpp
  7ZipInStreamWrapper.cpp
)

add_library(7zip STATIC ${lib7zip_SOURCES})
add_dependencies(7zip upstream)

target_compile_options(7zip PRIVATE -std=c++11)

if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  target_link_libraries(7zip -lole32 -loleaut32 -luuid)
  target_compile_options(7zip PRIVATE -DUNICODE -D_UNICODE)
endif() # Windows
