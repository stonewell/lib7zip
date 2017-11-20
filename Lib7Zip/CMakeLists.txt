cmake_minimum_required(VERSION 2.8.2)

project(lib7zip)

set (SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# ==> 7-zip source download start

if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
set(UPSTREAM_SRC_URL http://www.7-zip.org/a/7z1604-src.7z)
set(UPSTREAM_PATCH_CMD dos2unix ${CMAKE_CURRENT_SOURCE_DIR}/001_avoid_duplicate_guids.patch COMMAND patch --ignore-whitespace --strip 1 < ${CMAKE_CURRENT_SOURCE_DIR}/001_avoid_duplicate_guids.patch COMMAND unix2dos ${UPSTREAM_DIR}/CPP/Windows/FileIO.h)
else() # Windows
set(UPSTREAM_SRC_URL https://downloads.sourceforge.net/project/p7zip/p7zip/16.02/p7zip_16.02_src_all.tar.bz2)
set(UPSTREAM_PATCH_CMD )
endif() # not Windows

include(ExternalProject)
ExternalProject_Add(
  upstream
  URL ${UPSTREAM_SRC_URL}
  SOURCE_DIR ${UPSTREAM_DIR}
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
  PATCH_COMMAND ${UPSTREAM_PATCH_CMD}
)

set(UPSTREAM_INCLUDE_DIR ${UPSTREAM_DIR})

# <== 7-zip source download end

include_directories(
  ${UPSTREAM_INCLUDE_DIR}
  ${UPSTREAM_INCLUDE_DIR}/CPP
  ${UPSTREAM_INCLUDE_DIR}/CPP/myWindows
  ${UPSTREAM_INCLUDE_DIR}/CPP/include_windows
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
)

add_library(7zip STATIC ${lib7zip_SOURCES})
add_dependencies(7zip upstream)

set(ADDITIONAL_SOURCES
  # this is all a terrible hack, but it lets us get away with
  # a regular ExternalProject instead of using a trick like
  # https://github.com/Crascit/DownloadProject
  -I${UPSTREAM_INCLUDE_DIR}
  -I${UPSTREAM_INCLUDE_DIR}/CPP
  -I${UPSTREAM_INCLUDE_DIR}/CPP/myWindows
  -I${UPSTREAM_INCLUDE_DIR}/CPP/include_windows
  ${UPSTREAM_DIR}/CPP/Common/MyWindows.cpp
  ${UPSTREAM_DIR}/CPP/Windows/PropVariant.cpp
)
target_link_libraries(7zip ${ADDITIONAL_SOURCES})

target_compile_options(7zip PRIVATE -std=c++11)

if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  target_link_libraries(7zip -lole32 -loleaut32 -luuid)
  target_compile_options(7zip PRIVATE -DUNICODE -D_UNICODE)
else()
  target_link_libraries(7zip -ldl)
endif() # Windows
