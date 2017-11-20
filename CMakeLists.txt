cmake_minimum_required(VERSION 2.8)

project(lib7zip)

option(LIB7ZIP_BUILD_TESTS "Build tests for lib7zip" OFF)

set(UPSTREAM_DIR ${PROJECT_BINARY_DIR}/upstream-7zip)

add_subdirectory(Lib7Zip)

if (LIB7ZIP_BUILD_TESTS)
  add_subdirectory(Test7Zip)
endif()

