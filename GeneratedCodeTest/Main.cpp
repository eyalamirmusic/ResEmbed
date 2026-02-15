#include "ResourceData.h"
#include <iostream>

struct StaticLoader
{
    StaticLoader()
    {
        std::cout << Resources::get("data.txt").toString() << std::endl;
    }
};

StaticLoader loader;

int main()
{
    return 0;
}
