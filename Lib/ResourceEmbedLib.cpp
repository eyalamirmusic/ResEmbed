#include "ResourceEmbedLib.h"

namespace Resources
{
Map& getMap()
{
    static Map map;
    return map;
}

DataView get(const std::string& name)
{
    return {getMap()[name]};
}

Data::Data(const std::string& name, const RawData& rawData)
{
    getMap()[name] = rawData;
}
} // namespace Resources