add_library(ResourceEmbedLib INTERFACE)

target_sources(ResourceEmbedLib INTERFACE
        ResourceEmbedLib.cpp
        ResourceEmbedLib.h)

target_include_directories(ResourceEmbedLib INTERFACE ${CMAKE_CURRENT_LIST_DIR})