#include "BinaryData.h"
#include "Resources.h"
#include <iostream>

void printResource(const std::string& name, const std::string& category = "BinaryData")
{
    auto d = Resources::get(name, "Custom");
    std::cout << d.toString() << std::endl;
}

int main()
{
    printResource("data.bin");
    printResource("data2.txt");
    printResource("SRC-ND_INST-Clap_NAME-Shlap!.flac");
    printResource("data2.txt", "Resources");

    return 0;
}