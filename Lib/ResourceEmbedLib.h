#pragma once

#include <vector>
#include <map>
#include <span>
#include <string>

namespace Resources
{
using Storage = std::vector<unsigned char>;
using Map = std::map<std::string, Storage>;
using RawData = std::initializer_list<unsigned char>;
using View = std::span<const unsigned char>;

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

    std::string toString() const { return {asCharPointer(), size()}; };

    View dataView;
};

DataView get(const std::string& name);

struct Data
{
    Data(const std::string& name, const RawData& rawData);
};
} // namespace Resources