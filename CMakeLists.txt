cmake_minimum_required(VERSION 3.21)
project(ResEmbed)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(Lib)

add_subdirectory(Generator)

include(CMake/ResEmbed.cmake)

option(RESOURCE_EMBEDDER_TESTS "Build ResourceEmbedder tests" OFF)

if(PROJECT_IS_TOP_LEVEL OR RESOURCE_EMBEDDER_TESTS)
    add_subdirectory(Tests)
    add_subdirectory(Examples)
endif()