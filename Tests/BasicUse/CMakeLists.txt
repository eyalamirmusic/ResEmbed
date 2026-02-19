add_library(TestResources STATIC)
res_embed_add(TestResources DIRECTORY Resources NAMESPACE BinaryData CATEGORY "Custom")

add_executable(ResourceEmbedderTest main.cpp)
res_embed_add(ResourceEmbedderTest DIRECTORY MoreResources)
target_link_libraries(ResourceEmbedderTest PRIVATE TestResources)
