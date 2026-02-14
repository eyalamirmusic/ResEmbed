cmake_minimum_required(VERSION 4.1)
project(ResourceEmbedder)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(Lib)
add_subdirectory(Generator)
include(CMake/ResourceEmbed.cmake)

add_subdirectory(Test)