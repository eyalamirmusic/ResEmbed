# ResEmbed

A CMake/C++20 library for embedding binary files directly into your executables and libraries at compile time.

Resource files (images, text, shaders, etc.) are converted into C byte arrays during the build, then registered into a global, thread-safe registry accessible at runtime through a simple C++ API.

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
#include <Resources.h>
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

Both examples are available under `Examples/` in the repository.

## CMake API

```cmake
res_embed_add(<target>
    DIRECTORY <dir> | FILES <file1> [<file2> ...]
    [NAMESPACE <namespace>]
    [CATEGORY <category>]
)
```

### Parameters

| Parameter   | Required | Default       | Description                                           |
|-------------|----------|---------------|-------------------------------------------------------|
| `DIRECTORY` | Either   | -             | Recursively embed all files from a directory.         |
| `FILES`     | Either   | -             | Embed specific files by path.                         |
| `NAMESPACE` | No       | `Resources`   | C++ namespace for the generated header.               |
| `CATEGORY`  | No       | `Resources`   | Category string used to group resources at runtime.   |

`DIRECTORY` and `FILES` are mutually exclusive. When using `DIRECTORY`, files are discovered with `GLOB_RECURSE` and `CONFIGURE_DEPENDS`, so adding or removing files will automatically trigger a CMake reconfigure.

You can call `res_embed_add` multiple times on the same target with different namespaces:

```cmake
res_embed_add(MyApp DIRECTORY shaders NAMESPACE Shaders CATEGORY "Shaders")
res_embed_add(MyApp DIRECTORY textures NAMESPACE Textures CATEGORY "Textures")
```

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

Each `res_embed_add` call generates a header named `<Namespace>.h` (e.g., `Resources.h`, `Shaders.h`). You must `#include` this header in at least one translation unit in your target to register the resources. If a generated header is not included, the corresponding resources will not be available at runtime.

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

