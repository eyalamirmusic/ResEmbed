add_library(GeneratedCode INTERFACE)
target_sources(GeneratedCode INTERFACE Resource.c ResourceData.h ResourceData.cpp)
target_link_libraries(GeneratedCode INTERFACE ResEmbed)

add_executable(GeneratedCodeTest Main.cpp)
target_link_libraries(GeneratedCodeTest PRIVATE GeneratedCode)
