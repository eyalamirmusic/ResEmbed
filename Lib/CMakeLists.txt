add_library(ResourceEmbedLib INTERFACE)

target_sources(ResourceEmbedLib INTERFACE
        ResEmbed/ResEmbed.cpp
        ResEmbed/ResEmbed.h)

target_include_directories(ResourceEmbedLib INTERFACE ${CMAKE_CURRENT_LIST_DIR})
