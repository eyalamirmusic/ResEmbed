#include "Lib.h"

#include <iostream>
#include <ResEmbed/ResEmbed.h>

void printResources()
{
    if (auto res = ResEmbed::get("Text.txt"))
        std::cout << res.toString() << std::endl;
    else
        std::cout << "Missing Text.txt resource!";
}