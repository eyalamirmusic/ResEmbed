#include "ResourceEmbedLib.h"
#include <iostream>

void printResource(const std::string& name)
{
    auto d = Resources::get(name);
    std::cout << d.toString() << std::endl;
}

int main()
{
    printResource("data.bin");
    auto d = Resources::get("data2.txt");
    std::cout << d.toString() << std::endl;
    return 0;
}