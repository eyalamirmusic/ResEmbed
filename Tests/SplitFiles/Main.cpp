#include <ResEmbed/ResEmbed.h>
#include <iostream>

// Validates the SPLIT layout: each resource is emitted as its own extern "C"
// .c data file and stitched together by the generated registry. If the split
// codegen or the per-resource registration were wrong, the lookups below fail.
int main()
{
    auto hello = ResEmbed::get("Hello.txt");
    auto world = ResEmbed::get("World.txt");

    if (!hello || !world)
    {
        std::cerr << "Split resources missing!" << std::endl;
        return 1;
    }

    std::cout << hello.toStringView() << " / " << world.toStringView()
              << std::endl;

    return 0;
}
