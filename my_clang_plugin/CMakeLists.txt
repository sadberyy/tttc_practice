cmake_minimum_required(VERSION 3.13)
project(RenamePrefixPlugin)

find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)

include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${CLANG_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

add_library(RenamePrefixPlugin MODULE RenamePrefixPlugin.cpp)

set_target_properties(RenamePrefixPlugin PROPERTIES
  CXX_STANDARD 17
  PREFIX ""
  SUFFIX ".so"
)
