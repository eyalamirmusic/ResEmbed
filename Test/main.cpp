#include "ResourceEmbedLib.h"
#include <iostream>

int main()
{
    auto d = Resources::get("data");
    std::cout << d.toString() << std::endl;
    return 0;
}