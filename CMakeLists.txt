cmake_minimum_required(VERSION 3.2)

project(igzip-wrapper)

option(BUILD_TEST "Build inflate & deflate test executable" OFF)
option(MULTI_THREADED_DEFLATE "Use multi threaded for deflating" ON)

set(CMAKE_CXX_STANDARD 17)

find_library(ISAL_LIB isal REQUIRED)
find_path(IGZIP_INC isa-l/igzip_lib.h REQUIRED)
message("isa-l library found in ${ISAL_LIB}")
message("igzip headers found in ${IGZIP_INC}")

add_definitions("-O3" "-Wno-write-strings")
# if you want to change verbose level, uncomment it to build the library
# add_definitions("-D_IGZIP_VERBOSE_LEVEL=4")

if(${MULTI_THREADED_DEFLATE})
  find_package(Threads REQUIRED)
  add_definitions("-DHAVE_THREADS")
  message("enable multi threaded for deflating")
endif()

# igzip wrapper shared library
file(GLOB IGZIP_WRAP_SRC ${PROJECT_SOURCE_DIR}/*.c)
include_directories(${IGZIP_INC})
add_library(igzipwrap SHARED ${IGZIP_WRAP_SRC})
target_link_libraries(igzipwrap ${ISAL_LIB})
if(${MULTI_THREADED_DEFLATE})
  target_link_libraries(igzipwrap Threads::Threads)
endif()

if(${BUILD_TEST})
  # test inflate
  find_package(Threads REQUIRED)
  add_executable(inflate ${PROJECT_SOURCE_DIR}/test_inflate.cpp)
  target_link_libraries(inflate igzipwrap Threads::Threads)

  # test deflate
  add_executable(deflate ${PROJECT_SOURCE_DIR}/test_deflate.cpp)
  target_link_libraries(deflate igzipwrap)
endif()
