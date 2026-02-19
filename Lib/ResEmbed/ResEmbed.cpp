#include "ResEmbed.h"
#include "InternalStorage.h"

namespace ResEmbed
{
DataView::DataView(const View& viewToUse)
    : dataView(viewToUse) {}

DataView::Iterator DataView::begin() const { return dataView.begin(); }

DataView::Iterator DataView::end() const { return dataView.end(); }

const void* DataView::asRaw() const { return data(); }

const char* DataView::asCharPointer() const
{
    return static_cast<const char*>(asRaw());
}

const unsigned char* DataView::data() const { return dataView.data(); }

bool DataView::empty() const { return size() == 0; }
bool DataView::isValid() const { return !empty(); }

DataView::operator bool() const { return isValid(); }

size_t DataView::size() const { return dataView.size(); }

int DataView::getSize() const { return static_cast<int>(size()); }

std::string DataView::toString() const { return {asCharPointer(), size()}; }

DataView get(const std::string& name, const std::string& category)
{
    auto lock = std::lock_guard(Detail::getMutex());
    return {Detail::getMap()[category][name]};
}

ResourceMap getCategory(const std::string& category)
{
    auto lock = std::lock_guard(Detail::getMutex());
    return Detail::getMap()[category];
}
} // namespace ResEmbed