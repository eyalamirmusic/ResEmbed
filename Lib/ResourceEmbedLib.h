#pragma once

#include <map>
#include <span>
#include <string>
#include <vector>

namespace Resources
{
using View = std::span<const unsigned char>;
using ResourceMap = std::map<std::string, View>;
using CategoryMap = std::map<std::string, ResourceMap>;

inline constexpr auto DefaultCategory = "Resources";

struct Entry
{
    Entry() = default;

    Entry(const unsigned char* dataToUse,
          unsigned long sizeToUse,
          const char* nameToUse,
          const char* categoryToUse = "Resources")
        : data(dataToUse, sizeToUse), name(nameToUse), category(categoryToUse) {}

    View data;
    std::string name;
    std::string category;
};

using Entries = std::vector<Entry>;

struct DataView
{
    using Iterator = View::iterator;

    DataView() = default;

    DataView(const View& viewToUse)
        : dataView(viewToUse) {}

    Iterator begin() const { return dataView.begin(); }
    Iterator end() const { return dataView.end(); }

    const void* asRaw() const { return data(); }
    const char* asCharPointer() const { return static_cast<const char*>(asRaw()); }

    const unsigned char* data() const { return dataView.data(); }

    size_t size() const { return dataView.size(); }
    int getSize() const { return static_cast<int>(size()); }

    std::string toString() const { return {asCharPointer(), size()}; }

    View dataView;
};

DataView get(const std::string& name, const std::string& category = DefaultCategory);

ResourceMap& getCategory(const std::string& category);

void registerEntries(const Entries& entries);

struct Initializer
{
    Initializer(const Entries& entries)
    {
        registerEntries(entries);
    }
};
} // namespace Resources