cmake_minimum_required(VERSION 3.15)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 26)
project(PackageTest CXX)

find_package(rsl-config REQUIRED)

add_executable(rsl-config-test)
target_compile_options(rsl-config-test PRIVATE "-freflection-latest")
target_link_libraries(rsl-config-test PRIVATE rsl::config)

add_subdirectory(src)
target_compile_definitions(rsl-config-test PRIVATE RSL_TEST_NAMESPACE=rsl)
  
find_package(rsl-test REQUIRED)
target_link_libraries(rsl-config-test PRIVATE rsl::test rsl::test_main)
