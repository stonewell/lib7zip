cmake_minimum_required(VERSION 2.8.2)

project(lib7zip)

set (SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

include_directories(
  ${UPSTREAM_INCLUDE_DIR}
  ${UPSTREAM_INCLUDE_DIR}/CPP
  ${UPSTREAM_INCLUDE_DIR}/CPP/myWindows
  ${UPSTREAM_INCLUDE_DIR}/CPP/include_windows
)

if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  set(OSFUNCTIONS_SOURCE ${SOURCE_DIR}/OSFunctions_Win32.cpp)
else()
  set(OSFUNCTIONS_SOURCE ${SOURCE_DIR}/OSFunctions_UnixLike.cpp)
endif()

set (lib7zip_SOURCES
  ${SOURCE_DIR}/7ZipArchive.cpp
  ${SOURCE_DIR}/7ZipArchiveItem.cpp
  ${SOURCE_DIR}/7zipLibrary.cpp
  ${SOURCE_DIR}/HelperFuncs.cpp
  ${SOURCE_DIR}/SelfPath.cpp
  ${SOURCE_DIR}/7ZipArchiveOpenCallback.cpp
  ${SOURCE_DIR}/7ZipCodecInfo.cpp
  ${SOURCE_DIR}/7ZipCompressCodecsInfo.cpp
  ${SOURCE_DIR}/7ZipDllHandler.cpp
  ${SOURCE_DIR}/7ZipFormatInfo.cpp
  ${SOURCE_DIR}/7ZipObjectPtrArray.cpp
  ${SOURCE_DIR}/7ZipOpenArchive.cpp
  ${SOURCE_DIR}/7ZipInStreamWrapper.cpp
  ${OSFUNCTIONS_SOURCE}

  ${UPSTREAM_DIR}/CPP/Common/MyWindows.cpp
  ${UPSTREAM_DIR}/CPP/Windows/PropVariant.cpp
)

add_library(7zip STATIC ${lib7zip_SOURCES})

target_compile_options(7zip PRIVATE -std=c++11)

if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  target_link_libraries(7zip -lole32 -loleaut32 -luuid)
  target_compile_options(7zip PRIVATE -DUNICODE -D_UNICODE)
else()
  target_link_libraries(7zip -ldl)
  target_compile_options(7zip PRIVATE -fPIC)
endif() # Windows
