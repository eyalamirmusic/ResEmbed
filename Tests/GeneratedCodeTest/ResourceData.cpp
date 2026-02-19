#include <ResEmbed/ResEmbed.h>
#include <ResEmbed/Entries.h>

extern "C"
{
extern const unsigned char resource_0_data[];
extern const unsigned long resource_0_size;
}


namespace Resources
{
const ResEmbed::Entries& getResourceEntries()
{
    static const ResEmbed::Entries entries = {
        {resource_0_data, resource_0_size, "data.txt"}
    };

    return entries;
}
}
