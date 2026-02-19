cmake_minimum_required(VERSION 3.21)
project(ResourceEmbedder)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(Lib)
add_subdirectory(Generator)
include(CMake/ResourceEmbed.cmake)

option(RESOURCE_EMBEDDER_TESTS "Build ResourceEmbedder tests" OFF)

if(PROJECT_IS_TOP_LEVEL OR RESOURCE_EMBEDDER_TESTS)
    add_subdirectory(Tests)
endif()