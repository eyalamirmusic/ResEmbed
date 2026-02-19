#pragma once

#include "Common.h"
#include <map>

namespace ResEmbed
{
class DataView
{
public:
    using Iterator = View::iterator;

    DataView() = default;
    DataView(const View& viewToUse);

    Iterator begin() const;
    Iterator end() const;

    const void* asRaw() const;
    const char* asCharPointer() const;

    const unsigned char* data() const;

    bool empty() const;
    bool isValid() const;

    explicit operator bool() const;

    size_t size() const;
    int getSize() const;

    std::string toString() const;

private:
    View dataView;
};

using ResourceMap = std::map<std::string, DataView>;
using CategoryMap = std::map<std::string, ResourceMap>;

DataView get(const std::string& name, const std::string& category = defaultCategory);

ResourceMap getCategory(const std::string& category);
} // namespace ResEmbed