cmake_minimum_required(VERSION 3.29)
project(ResEmbed)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_LINK_LIBRARIES_STRATEGY REORDER_FREELY)

option(RESEMBED_HIDDEN_VISIBILITY
        "Compile ResEmbed with hidden symbol visibility" ON)

if(RESEMBED_HIDDEN_VISIBILITY)
    set(CMAKE_C_VISIBILITY_PRESET hidden)
    set(CMAKE_CXX_VISIBILITY_PRESET hidden)
    set(CMAKE_OBJC_VISIBILITY_PRESET hidden)
    set(CMAKE_OBJCXX_VISIBILITY_PRESET hidden)
    set(CMAKE_VISIBILITY_INLINES_HIDDEN TRUE)
endif()

add_subdirectory(Lib)

add_subdirectory(Generator)

include(CMake/ResEmbed.cmake)

option(RESOURCE_EMBEDDER_TESTS "Build ResourceEmbedder tests" OFF)

if(PROJECT_IS_TOP_LEVEL OR RESOURCE_EMBEDDER_TESTS)
    if(MSVC)
        add_compile_options(/W4)
    else()
        add_compile_options(-Wall -Wextra -Wpedantic)
    endif()

    add_subdirectory(Tests)
    add_subdirectory(Examples)
endif()