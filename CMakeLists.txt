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

option(BUILD_TESTING "Enable tests" ON)
option(ENABLE_COVERAGE "Enable coverage instrumentation" OFF)

if (BUILD_TESTING)
  message(STATUS "Building unit tests")

  add_executable(rsl_config_test)
  target_link_libraries(rsl_config_test PRIVATE rsl_config)
  
  add_subdirectory(test)
  target_compile_definitions(rsl_config_test PRIVATE RSL_TEST_NAMESPACE=rsl::_test)
  
  find_package(rsl-test REQUIRED)
  target_link_libraries(rsl_config_test PRIVATE rsl::test rsl::test_main)
  
  enable_testing()
  add_test(NAME rsl_config_test COMMAND rsl_config_test)

  if(ENABLE_COVERAGE)
    message(STATUS "Instrumenting for coverage")  

    target_compile_options(rsl_config PRIVATE -g -O0 --coverage)
    target_link_libraries(rsl_config PRIVATE --coverage)
  endif()
endif()
