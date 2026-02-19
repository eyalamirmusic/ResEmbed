#pragma once

#include <span>
#include <string>

namespace ResEmbed
{
using View = std::span<const unsigned char>;

inline constexpr auto defaultCategory = "Resources";
}