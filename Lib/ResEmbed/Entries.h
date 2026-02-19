#pragma once

#include "Common.h"
#include <vector>

namespace ResEmbed
{
struct Entry
{
    Entry() = default;

    Entry(const unsigned char* dataToUse,
          unsigned long sizeToUse,
          const char* nameToUse,
          const char* categoryToUse = defaultCategory);

    View data;
    std::string name;
    std::string category;
};

using Entries = std::vector<Entry>;

struct Initializer
{
    Initializer(const Entries& entries);
};
}