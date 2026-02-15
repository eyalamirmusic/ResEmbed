#include "ResourceEmbedLib.h"

namespace Resources
{
CategoryMap& getMap()
{
    static CategoryMap map;
    return map;
}

DataView get(const std::string& name, const std::string& category)
{
    return {getCategory(category)[name]};
}

ResourceMap& getCategory(const std::string& category)
{
    return getMap()[category];
}

void registerEntries(const Entries& entries)
{
    for (auto& entry: entries)
        getCategory(entry.category)[entry.name] = entry.data;
}
} // namespace Resources
