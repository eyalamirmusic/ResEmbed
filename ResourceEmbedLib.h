#pragma once

#include <array>
#include <map>
#include <span>
#include <string>

namespace Resources
{
    using Data = std::span<const unsigned char>;
    using Map = std::map<std::string, Data>;

    inline Map& getMap()
    {
        static Map map;
        return map;
    }

    inline Data get(const std::string& name) { return getMap().at(name); }
    inline void set(std::string name, const Data& data) { getMap()[std::move(name)] = data; }

    template <std::size_t N>
    struct Array
    {
        std::array<unsigned char, N> storage;

        Array(std::string name, std::array<unsigned char, N> arr) : storage(std::move(arr))
        {
            set(std::move(name), {storage.data(), storage.size()});
        }
    };

    template <std::size_t N>
    Array(std::string, std::array<unsigned char, N>) -> Array<N>;
} // namespace Resources
