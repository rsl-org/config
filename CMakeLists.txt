cmake_minimum_required(VERSION 3.15)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 26)

project(rsl_config CXX)
add_library(rsl_config)

set_target_properties(rsl_config PROPERTIES OUTPUT_NAME rsl_config)
target_compile_options(rsl_config PUBLIC 
  "-stdlib=libc++"
  "-freflection"
  "-fannotation-attributes"
  "-fparameter-reflection"
  "-fexpansion-statements"
  "-fconstexpr-backtrace-limit=0"
  # "-Wno-c++26-extensions"
  # "-ftime-trace"
  # "-fconstexpr-steps=10000000" # required to scan the global namespace
)
target_link_options(rsl_config PUBLIC "-stdlib=libc++" "-lc++abi")
target_include_directories(rsl_config PUBLIC 
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>)

find_package(rsl-util REQUIRED)
target_link_libraries(rsl_config PUBLIC rsl::util)

add_subdirectory(src)

install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/ DESTINATION include)
install(TARGETS rsl_config)

option(BUILD_EXAMPLES "Enable examples" ON)

if (BUILD_EXAMPLES)
  add_subdirectory(example)
endif()