#include <ResEmbed/ResEmbed.h>
#include <iostream>

// Validates the bucket layout for a build-time discovery mode (DIRECTORY):
// the directory's resources are round-robined across a fixed number of C
// translation units, and every one must still resolve at runtime regardless
// of how many buckets they were spread across.
int main()
{
    auto all = ResEmbed::getCategory(ResEmbed::defaultCategory);

    if (all.size() != 5)
    {
        std::cerr << "Expected 5 resources, got " << all.size() << std::endl;
        return 1;
    }

    for (auto& [name, data]: all)
        std::cout << name << "=" << data.toStringView() << " ";

    std::cout << std::endl;

    return 0;
}
