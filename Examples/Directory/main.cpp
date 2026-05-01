#include <ResEmbed/ResEmbed.h>
#include <iostream>

int main()
{
    auto category = ResEmbed::getCategory(ResEmbed::defaultCategory);

    for (auto& res: category)
    {
        auto name = res.first;
        auto content = res.second.toString();
        std::cout << "Name: " << name << " Content: " << content << std::endl;
    }

    return 0;
}