#include "BinaryData.h"
#include <iostream>

void printResource(const std::string& name)
{
    auto d = Resources::get(name, "Custom");
    std::cout << d.toString() << std::endl;
}

int main()
{
    printResource("data.bin");
    printResource("data2.txt");

    return 0;
}