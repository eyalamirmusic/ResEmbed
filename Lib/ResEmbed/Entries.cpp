#include "Entries.h"
#include "InternalStorage.h"

namespace ResEmbed
{
Entry::Entry(const unsigned char* dataToUse,
             unsigned long sizeToUse,
             const char* nameToUse,
             const char* categoryToUse)
    : data(dataToUse, sizeToUse), name(nameToUse), category(categoryToUse) {}


void registerEntries(const Entries& entries)
{
    auto lock = std::lock_guard(Detail::getMutex());

    for (auto& entry: entries)
        Detail::getMap()[entry.category][entry.name] = entry.data;
}


Initializer::Initializer(const Entries& entries)
{
    registerEntries(entries);
}
}