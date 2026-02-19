add_library(ResEmbed INTERFACE)

target_sources(ResEmbed INTERFACE
        ResEmbed/Entries.cpp
        ResEmbed/Entries.h
        ResEmbed/InternalStorage.cpp
        ResEmbed/InternalStorage.h
        ResEmbed/ResEmbed.cpp
        ResEmbed/ResEmbed.h
        )

target_include_directories(ResEmbed INTERFACE ${CMAKE_CURRENT_LIST_DIR})
