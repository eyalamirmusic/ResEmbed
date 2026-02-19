#pragma once

#include <ResEmbed/ResEmbed.h>

namespace ResEmbed
{
const Entries& getResourceEntries();
static const Initializer resourceInitializer {getResourceEntries()};
}