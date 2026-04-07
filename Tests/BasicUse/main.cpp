#include <BinaryData.h>
#include "Resources.h"
#include <iostream>

static void printResource(const std::string& name, const std::string& category)
{
    auto d = ResEmbed::get(name, category);
    std::cout << d.toStringView() << std::endl;
}

int main()
{
    printResource("data.bin", "Custom");
    printResource("data2.txt", "Custom");
    printResource("data2.txt", "Resources");

    return 0;
}