add_executable(SplitFilesTest Main.cpp)

# FILES mode emits one .c translation unit per resource by default.
res_embed_add(SplitFilesTest
        FILES Resources/Hello.txt Resources/World.txt
        NAMESPACE SplitData)

# Exercise the unity-build path too: CMake coalesces the per-resource .c
# files into a single unity TU. This target validates that the generated
# split sources participate correctly in unity builds.
add_executable(SplitFilesUnityTest Main.cpp)
res_embed_add(SplitFilesUnityTest
        FILES Resources/Hello.txt Resources/World.txt
        NAMESPACE SplitData)
set_target_properties(SplitFilesUnityTest PROPERTIES
        UNITY_BUILD ON
        UNITY_BUILD_BATCH_SIZE 0)
