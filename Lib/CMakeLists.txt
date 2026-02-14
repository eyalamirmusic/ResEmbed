add_library(ResourceEmbedLib STATIC)

target_sources(ResourceEmbedLib PRIVATE
        ResourceEmbedLib.cpp
        ResourceEmbedLib.h)

target_include_directories(ResourceEmbedLib PUBLIC ${CMAKE_CURRENT_LIST_DIR})