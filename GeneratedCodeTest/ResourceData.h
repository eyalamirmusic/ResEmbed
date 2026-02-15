#pragma once

#include "ResourceEmbedLib.h"

extern const unsigned char resource_0_data[];
extern const size_t resource_0_size;

namespace Resources
{
static const Entries resource_entries = {
    {resource_0_data, resource_0_size, "data.txt"}
};

static const Initializer resourceInitializer {resource_entries};
}