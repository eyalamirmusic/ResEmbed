# ResEmbed

A CMake/C++20 library for embedding binary files directly into your executables and libraries at compile time.

Resource files (images, text, shaders, etc.) are converted into C byte arrays during the build, then registered into a global, thread-safe registry accessible at runtime through a simple C++ API.

ResEmbed is meant to be extremelly portable:
Currently tested on MacOS/iOS/Linux GCC/Linux Clang/Windows MSVC and Windows Clang.

## Integration

```cmake
include(FetchContent)

FetchContent_Declare(ResEmbed
    GIT_REPOSITORY https://github.com/eyalamirmusic/ResEmbed
    GIT_TAG main)

FetchContent_MakeAvailable(ResEmbed)

add_executable(App Main.cpp)
res_embed_add(App DIRECTORY Resources)
```

## Basic Usage

### Embedding a single file

**CMakeLists.txt:**

```cmake
add_executable(MyProject Main.cpp)
res_embed_add(MyProject FILES "Resource.txt")
```

**Main.cpp:**

```cpp
#include <ResEmbed/ResEmbed.h>
#include <iostream>

int main()
{
    std::cout << ResEmbed::get("Resource.txt").toString();
    return 0;
}
```

### Embedding a directory

**CMakeLists.txt:**

```cmake
add_executable(MyProject main.cpp)
res_embed_add(MyProject DIRECTORY Resources)
```

### Runtime resources with a shared library

A library can use `ResEmbed::get()` to access resources at runtime without embedding them itself. The application that links against it provides the actual resource files. This lets you reuse the same library with different resources per application.

**Lib.cpp** (the shared library):

```cpp
#include <ResEmbed/ResEmbed.h>
#include <iostream>

void printResources()
{
    if (auto res = ResEmbed::get("Text.txt"))
        std::cout << res.toString() << std::endl;
    else
        std::cout << "Missing Text.txt resource!";
}
```

**CMakeLists.txt:**

```cmake
#Lib only needs to link with ResEmbed
add_library(Lib STATIC Lib.cpp)
target_link_libraries(Lib PRIVATE ResEmbed)

# AppA embeds ResourcesA/Text.txt
add_executable(AppA Main.cpp)
res_embed_add(AppA DIRECTORY ResourcesA)
target_link_libraries(AppA PRIVATE Lib)

#AppB embeds ResourcesB/Text.txt
add_executable(AppB Main.cpp)
res_embed_add(AppB DIRECTORY ResourcesB)
target_link_libraries(AppB PRIVATE Lib)
```

Both apps share the same library code, but each provides its own `Text.txt`. The library checks for the resource with `operator bool` and handles the missing case gracefully.

All examples are available under `Examples/` in the repository.

## CMake API

```cmake
res_embed_add(<target>
    FILES <file1> [<file2> ...]              # explicit list (configure-time)
    | MANIFEST <file>                        # newline-separated list (build-time)
    | SCAN_DIR <dir>                         # recursive walk (build-time)
    | DIRECTORY <dir>                        # back-compat alias for SCAN_DIR
    [NAMESPACE <namespace>]
    [CATEGORY <category>]
    [BASE_DIRECTORY <dir>]
    [DEPENDS <target-or-file> ...]
    [TU_COUNT <n>]
)
```

### Parameters

| Parameter        | Required | Default       | Description                                                                                          |
|------------------|----------|---------------|------------------------------------------------------------------------------------------------------|
| `FILES`          | Mode     | -             | Explicit list of files. The set is fixed at configure time; contents are watched via a depfile.      |
| `MANIFEST`       | Mode     | -             | Path to a newline-separated list of files, typically produced by an upstream rule at build time.     |
| `SCAN_DIR`       | Mode     | -             | Recursively walk a directory at build time. Combine with `DEPENDS` for build-output directories.     |
| `DIRECTORY`      | Mode     | -             | Back-compat alias for `SCAN_DIR` with an extra `CONFIGURE_DEPENDS` glob for add/remove detection.    |
| `NAMESPACE`      | No       | `Resources`   | C++ namespace for the generated header and basename of the generated files.                          |
| `CATEGORY`       | No       | `Resources`   | Category string used to group resources at runtime.                                                  |
| `BASE_DIRECTORY` | No       | -             | When set, each resource is keyed by its path relative to this directory (e.g. `assets/index.js`). Without it, keys are the basename only. |
| `DEPENDS`        | No       | -             | Extra build-graph dependencies — typically a stamp produced by the rule that fills `SCAN_DIR` or writes `MANIFEST`. |
| `TU_COUNT`       | No       | per-resource (FILES) / 8 (others) | Number of `.c` data translation units to emit. See [Translation-unit layout](#translation-unit-layout-and-unity-builds). |

Exactly one of `FILES`, `MANIFEST`, `SCAN_DIR`, or `DIRECTORY` must be specified.

You can call `res_embed_add` multiple times on the same target with different namespaces:

```cmake
res_embed_add(MyApp DIRECTORY shaders NAMESPACE Shaders CATEGORY "Shaders")
res_embed_add(MyApp DIRECTORY textures NAMESPACE Textures CATEGORY "Textures")
```

### How discovery works

The four modes differ in *when* the list of files is determined and how changes propagate.

**`FILES`** — configure-time list. The CMake call writes a manifest file containing the absolute paths you passed. The build then reads that manifest. Adding or removing entries from `FILES` requires re-running CMake (which is fine — `CMakeLists.txt` itself changed). Editing the contents of any listed file does **not** require a reconfigure: the generator emits a [Make-format depfile](https://www.gnu.org/software/make/manual/html_node/Generating-Prerequisites-Automatically.html) and Ninja re-runs the embedding step automatically.

**`MANIFEST`** — build-time list, written by something else. The manifest is a newline-separated file of paths. The embedding step depends on the manifest, so whenever an upstream rule rewrites the manifest, embedding re-runs. Use this when another build step *already* knows exactly which files it produced (e.g. a code generator emitting a `.manifest` next to its outputs).

**`SCAN_DIR`** — build-time directory walk. The generator runs `recursive_directory_iterator` over the directory each time it executes. The set of files is whatever exists on disk at that moment. Because the scan happens during the build, you almost always pair `SCAN_DIR` with `DEPENDS <stamp>`, where `<stamp>` is touched by the rule that *fills* the directory:

```cmake
add_custom_command(
    OUTPUT  "${MY_STAMP}"
    COMMAND <build-step-that-populates-dist-dir>
    COMMAND ${CMAKE_COMMAND} -E touch "${MY_STAMP}"
    DEPENDS <input-sources>)

res_embed_add(MyApp
    SCAN_DIR       "${DIST_DIR}"
    BASE_DIRECTORY "${DIST_DIR}"
    DEPENDS        "${MY_STAMP}"
    NAMESPACE      WebResources)
```

Whenever the upstream rule touches `MY_STAMP`, the generator re-scans the directory, picks up whatever files exist, and re-emits the registry — all inside a single `cmake --build` invocation. No configure-time globbing of build outputs, and no two-build dance.

**`DIRECTORY`** — back-compat alias for `SCAN_DIR`, with an added configure-time `CONFIGURE_DEPENDS` glob that re-triggers CMake when files are added or removed from a *static* directory you check into source control. Use this for fixed asset folders (`shaders/`, `textures/`); use `SCAN_DIR` + `DEPENDS` for build outputs.

### Generated files and rebuild semantics

A single `res_embed_add(<target> NAMESPACE <NS> ...)` call emits these files under the target's binary directory in `<target>-<NS>-Generated/`:

- `<NS>.cpp` — in the split layout, just `getResourceEntries()` referencing the data arrays via `extern`; in the combined (C++-only) fallback, the byte arrays as well.
- `<NS>_<i>.c` — split layout: the data translation units, each an `extern "C"` byte array per resource (see [Translation-unit layout](#translation-unit-layout-and-unity-builds)).
- `<NS>.h` — declares `getResourceEntries()` for programmatic access.
- `<NS>_Register.cpp` — anonymous-namespace static initializer that registers the entries into the runtime map.

Alongside them the generator writes `<NS>.d`, a Make-format depfile listing every file consumed by the last run. Ninja reads this via the custom command's `DEPFILE` clause and re-runs the embed step whenever one of those files changes — without involving CMake at all. The depfile is what makes content edits cheap: only the bytes that actually changed flow through the rebuild.

The list of files itself (which files exist, not what's in them) is tracked through the chosen mode:

- `FILES`: changes when you edit `CMakeLists.txt` → CMake reconfigure.
- `MANIFEST`: changes when an upstream rule rewrites the manifest → embed step rebuilds via its `DEPENDS`.
- `SCAN_DIR`: changes when the directory's contents change → embed step rebuilds via the `DEPENDS <stamp>` you pass.
- `DIRECTORY`: as above, plus a `CONFIGURE_DEPENDS` glob that reconfigures CMake when files appear or vanish.

### Translation-unit layout (and unity builds)

The embedded bytes are emitted as **plain-C `.c` files** (`<NS>_0.c`, `<NS>_1.c`, …), each defining `extern "C"` byte arrays, plus a small `<NS>.cpp` registry that stitches them together via `extern` declarations. This holds in *every* discovery mode, and it's deliberate:

- **The bytes are C, not C++.** Large brace-initializer arrays are compiled by the C front end, which digests them far faster than C++. (This is the same property the README criticizes JUCE's `BinaryData` for, below.)
- **It's parallel and granular.** The `.c` files compile concurrently, and editing one resource only re-touches the `.c` it lives in.

Resources are round-robined across **N** data TUs, and how `N` is chosen is the only difference between modes:

- **`FILES` mode** knows the resource count when CMake configures, so `N` defaults to **one `.c` per resource** — the finest grain, so a content edit recompiles only that resource's `.c`.

- **`SCAN_DIR` / `MANIFEST` / `DIRECTORY` modes** resolve their file list at *build* time, so CMake can't size `N` to the (unknown) resource count. `N` defaults to **8**. Resources are spread across those 8 buckets; if there are fewer resources than buckets, the surplus `.c` files are empty (and compile instantly).

`TU_COUNT <n>` overrides `N` in any mode. In `FILES` mode it's a cap (never more TUs than resources); in the build-time modes it sets the bucket count outright:

```cmake
res_embed_add(MyApp SCAN_DIR "${DIST_DIR}" BASE_DIRECTORY "${DIST_DIR}"
              DEPENDS "${STAMP}" TU_COUNT 16)   # 16 C buckets
```

> If the consuming project never enabled the `C` language, all modes fall back to a single combined `<NS>.cpp` compiled as C++, so C++-only projects keep building.

#### Coalescing the TUs with unity builds

To go the other way — fewer, bigger TUs — there's no bespoke flag; the generated `.c` files are ordinary target sources, so CMake's native [unity builds](https://cmake.org/cmake/help/latest/prop_tgt/UNITY_BUILD.html) coalesce them with no extra wiring:

```cmake
add_executable(MyApp Main.cpp)
res_embed_add(MyApp FILES logo.png font.ttf shader.glsl)

# Compile all the data .c files as a single unity TU:
set_target_properties(MyApp PROPERTIES UNITY_BUILD ON UNITY_BUILD_BATCH_SIZE 0)
```

Use `UNITY_BUILD_BATCH_SIZE <n>` to batch in groups of `n`, or set `CMAKE_UNITY_BUILD=ON` project-wide. You get the single-TU compile profile back when you want it, while the default stays granular and parallel.

### Driving a code-generator → embedder chain

A common pattern: a code-generator emits files into a directory, and you want everything in that directory embedded. Wire it as:

```cmake
set(GEN_STAMP "${CMAKE_CURRENT_BINARY_DIR}/codegen.stamp")
add_custom_command(
    OUTPUT  "${GEN_STAMP}"
    COMMAND ${MY_CODEGEN_TOOL} --out "${GEN_OUT_DIR}"
    COMMAND ${CMAKE_COMMAND} -E touch "${GEN_STAMP}"
    DEPENDS <inputs that feed the codegen>)

res_embed_add(MyApp
    SCAN_DIR       "${GEN_OUT_DIR}"
    BASE_DIRECTORY "${GEN_OUT_DIR}"
    DEPENDS        "${GEN_STAMP}"
    NAMESPACE      Generated)
```

The graph is fully build-time: edit a codegen input → stamp updates → embed rescans → only the .cpp recompiles. One `cmake --build` produces a working binary even from a clean build tree.

If your code generator already emits a list of its outputs (a `manifest.txt`), prefer `MANIFEST` over `SCAN_DIR` — the manifest itself is the dependency edge, and you don't need a separate stamp file.

## C++ API

All runtime functions are in the `ResEmbed` namespace. The `<ResEmbed/ResEmbed.h>` header is linked automatically when using `res_embed_add`.

### `ResEmbed::get`

Retrieve a single resource by name (and optionally category):

```cpp
auto data = ResEmbed::get("image.png");
auto data = ResEmbed::get("image.png", "Textures");
```

### `ResEmbed::getCategory`

Retrieve all resources in a category:

```cpp
auto resources = ResEmbed::getCategory("Textures");

for (auto& [name, data] : resources)
    std::cout << name << ": " << data.size() << " bytes" << std::endl;
```

Each `res_embed_add` call also generates a small registration source file that the CMake function attaches to the target automatically. You do **not** need to include any generated header for resources to be available at runtime — `#include <ResEmbed/ResEmbed.h>` is enough.

When `res_embed_add` is called on a static library, the registration source is propagated as an `INTERFACE` source so that any executable (or shared library) linking the static library compiles the registration into the final binary. This avoids the linker dead-stripping the static initializer that performs registration.

If you want programmatic access to the entries for a given namespace (advanced use), include the generated `<Namespace>.h` and call `<Namespace>::getResourceEntries()`.

### Alternatives
There are many existing similar solutions to this problem.
I will explore a couple that I know, and why I chose my implementation instead:

### #embed
C++26 brought in #embed that lets you embed resource quickly.
It's awesome, and you should be using it, but other than the demand for a very modern compiler
it's also a strict compile time API. 

One of my design goals was that you can create shared libraries that 'require' resources.
For example, you can now have a shared library that calls

```cpp
auto res = ResEmbed::Get("BackgroundImage.png");
```
And link multiple products against it, each with a different background image.

### JUCE's BinaryData
JUCE's BinaryData was a big influence on this design
Since it's something I use in production in audio software.

There are three main problems with the JUCE one:
1. It requires JUCE
2. It doesn't have a dyanamic runtime API, just a static one
3. It's using C++ instead of C for the resources, which makes it a bit slow

### vector-of-bool cmrc
https://github.com/vector-of-bool/cmrc

This library is quite awesome and uses CMake only and has a good runtime API, 
but it was just too slow to compile for my taste.

