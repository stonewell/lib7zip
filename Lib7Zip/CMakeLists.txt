cmake_minimum_required(VERSION 3.12)

project(lib7zip)

set (SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

include_directories(
  ${UPSTREAM_INCLUDE_DIR}
  ${UPSTREAM_INCLUDE_DIR}/CPP
  ${UPSTREAM_INCLUDE_DIR}/CPP/Common
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
add_custom_command(OUTPUT ${lib7zip_SOURCES} DEPENDS upstream-7zip COMMAND "")
add_library(7zip STATIC ${lib7zip_SOURCES})

target_compile_options(7zip PRIVATE)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  # -pie causes executable problems: the tests simply don't run
  # -flto causes linking probems
  # -fstack-protector-strong causes crash, even just -fstack-protector causes (at least on Windows, MinGW w64)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wconversion -Wformat -Werror=format-security")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-strict-aliasing -mmitigate-rop -fno-common -fstack-check")
  add_definitions("-D_FORTIFY_SOURCE=2")
  
  if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcet -fsanitize=cfi")
  endif()
  set_target_properties(fuse_7z_ng PROPERTIES LINK_FLAGS "-Wl,-O1,--sort-common,--as-needed,--dynamicbase,--nxcompat,--export-all-symbols,-flto")
  
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set_target_properties(fuse_7z_ng PROPERTIES LINK_FLAGS "-Wl,--image-base,0x140000000")
  endif()
elseif(MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /sdl /GS /SafeSEH /NXCOMPAT /dynamicbase /guard:cf /HIGHENTROPYVA")
  set_target_properties(fuse_7z_ng PROPERTIES LINK_FLAGS "/guard:cf")
else()
  message(FATAL_ERROR "The compiler is not supported")
endif()

set_property(TARGET 7zip PROPERTY CXX_STANDARD 20)

if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  target_link_libraries(7zip ole32 oleaut32 uuid)
  target_compile_options(7zip PRIVATE -DUNICODE -D_UNICODE)
else()
  target_link_libraries(7zip -ldl)
  target_compile_options(7zip PRIVATE -fPIC)
endif() # Windows
