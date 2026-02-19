#pragma once

#include <ResEmbed/ResEmbed.h>

namespace Resources
{
const ResEmbed::Entries& getResourceEntries();
static const ResEmbed::Initializer resourceInitializer {getResourceEntries()};
}