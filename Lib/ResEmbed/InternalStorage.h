#pragma once

#include <mutex>
#include "ResEmbed.h"

namespace ResEmbed::Detail
{
std::mutex& getMutex();
CategoryMap& getMap();
}