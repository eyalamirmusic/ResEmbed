#include "ResourceEmbedLib.h"
#include <mutex>

namespace Resources
{
std::mutex mutex;

CategoryMap& getMap()
{
    static CategoryMap map;
    return map;
}

DataView get(const std::string& name, const std::string& category)
{
    auto lock = std::lock_guard(mutex);
    return {getMap()[category][name]};
}

ResourceMap getCategory(const std::string& category)
{
    auto lock = std::lock_guard(mutex);
    return getMap()[category];
}

void registerEntries(const Entries& entries)
{
    auto lock = std::lock_guard(mutex);

    for (auto& entry: entries)
        getMap()[entry.category][entry.name] = entry.data;
}

Initializer::Initializer(const Entries& entries)
{
    registerEntries(entries);
}
} // namespace Resources