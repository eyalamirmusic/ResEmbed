cmake_minimum_required(VERSION 4.1)
project(ResourceEmbedder)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(Lib)
add_subdirectory(Generator)
include(cmake/ResourceEmbed.cmake)

add_executable(ResourceEmbedder main.cpp)
target_link_libraries(ResourceEmbedder PRIVATE ResourceEmbedLib)
embed_resources(ResourceEmbedder testdata/data.bin)