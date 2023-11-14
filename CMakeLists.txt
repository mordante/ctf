cmake_minimum_required(VERSION 3.28.0 FATAL_ERROR)
project(ct_format LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS NO)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-stdlib=libc++>)
add_link_options($<$<COMPILE_LANGUAGE:CXX>:-stdlib=libc++>)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
include(ExternalProjects)

add_subdirectory(include)
add_subdirectory(scripts)
add_subdirectory(test)
add_subdirectory(benchmark)
