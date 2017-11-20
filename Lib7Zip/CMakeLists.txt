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
  ${SOURCE_DIR}/7ZipArchive.cpp
  ${SOURCE_DIR}/7ZipArchiveItem.cpp
  ${SOURCE_DIR}/7zipLibrary.cpp
  ${SOURCE_DIR}/HelperFuncs.cpp
  ${SOURCE_DIR}/7ZipArchiveOpenCallback.cpp
  ${SOURCE_DIR}/7ZipCodecInfo.cpp
  ${SOURCE_DIR}/7ZipCompressCodecsInfo.cpp
  ${SOURCE_DIR}/7ZipDllHandler.cpp
  ${SOURCE_DIR}/7ZipFormatInfo.cpp
  ${SOURCE_DIR}/7ZipObjectPtrArray.cpp
  ${SOURCE_DIR}/7ZipOpenArchive.cpp
  ${SOURCE_DIR}/OSFunctions_UnixLike.cpp
  ${SOURCE_DIR}/OSFunctions_Win32.cpp
  ${SOURCE_DIR}/7ZipInStreamWrapper.cpp

  ${UPSTREAM_DIR}/CPP/Common/MyWindows.cpp
  ${UPSTREAM_DIR}/CPP/Windows/PropVariant.cpp
)

add_library(7zip STATIC ${lib7zip_SOURCES})
add_dependencies(7zip upstream)

target_compile_options(7zip PRIVATE -std=c++11)

if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  target_link_libraries(7zip -lole32 -loleaut32 -luuid)
  target_compile_options(7zip PRIVATE -DUNICODE -D_UNICODE)
endif() # Windows
