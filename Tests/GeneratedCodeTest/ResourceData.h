#pragma once

#include <ResEmbed/Entries.h>
#include <ResEmbed/ResEmbed.h>

namespace Resources
{
const ResEmbed::Entries& getResourceEntries();
static const ResEmbed::Initializer resourceInitializer {getResourceEntries()};
}