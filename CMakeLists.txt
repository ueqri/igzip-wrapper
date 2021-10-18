cmake_minimum_required(VERSION 3.2)

project(igzip-wrapper)

set(CMAKE_CXX_STANDARD 17)

find_library(ISAL_LIB isal REQUIRED)
find_path(IGZIP_INC isa-l/igzip_lib.h REQUIRED)
message("isa-l library found in ${ISAL_LIB}")
message("igzip headers found in ${IGZIP_INC}")

find_package(Threads REQUIRED)

add_definitions("-O3" "-Wno-write-strings")
add_definitions("-DHAVE_THREADS")
include_directories(${PROJECT_SOURCE_DIR})

# igzip wrapper shared library
file(GLOB IGZIP_WRAP_SRC ${PROJECT_SOURCE_DIR}/*.c)
include_directories(${IGZIP_INC})
add_library(igzipwrap SHARED ${IGZIP_WRAP_SRC})
target_link_libraries(igzipwrap Threads::Threads ${ISAL_LIB})

# igzip test inflate
add_executable(inflate ${PROJECT_SOURCE_DIR}/test_inflate.cpp)
target_link_libraries(inflate igzipwrap)

# igzip test deflate
add_executable(deflate ${PROJECT_SOURCE_DIR}/test_deflate.cpp)
target_link_libraries(deflate igzipwrap)
