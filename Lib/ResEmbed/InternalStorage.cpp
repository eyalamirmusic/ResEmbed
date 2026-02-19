#include "InternalStorage.h"

namespace ResEmbed::Detail
{
CategoryMap& getMap()
{
    static auto map = CategoryMap();
    return map;
}

std::mutex& getMutex()
{
    static auto mutex = std::mutex();
    return mutex;
}
}