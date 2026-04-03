cmake_minimum_required(VERSION 3.15)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 26)

project(rsl-config CXX)
add_library(rsl-config)

set_target_properties(rsl-config PROPERTIES OUTPUT_NAME rsl-config)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  target_compile_options(rsl-config PUBLIC "-stdlib=libc++" "-freflection-latest")
  target_link_options(rsl-config PUBLIC "-stdlib=libc++")
  target_link_libraries(rsl-config PUBLIC "c++abi")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_compile_options(rsl-config PUBLIC "-freflection")
endif()

target_include_directories(rsl-config PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>)

find_package(rsl-util REQUIRED)
target_link_libraries(rsl-config PUBLIC rsl::util)

add_subdirectory(src)

install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/ DESTINATION include)
install(TARGETS rsl-config)

option(BUILD_EXAMPLES "Enable examples" ON)

if (BUILD_EXAMPLES)
  add_subdirectory(example)
endif()
