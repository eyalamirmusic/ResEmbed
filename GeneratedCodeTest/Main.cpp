#include "ResourceEmbedLib.h"
#include <iostream>

struct StaticChecker
{
    StaticChecker()
    {
        std::cout << Resources::get("data.txt").toString() << std::endl;
    }
};

StaticChecker checker;

int main()
{
    return 0;
}