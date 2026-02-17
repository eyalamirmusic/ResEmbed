#pragma once

#include "ResourceEmbedLib.h"

namespace Resources
{
const Entries& getResourceEntries();
static const Initializer resourceInitializer {getResourceEntries()};
}