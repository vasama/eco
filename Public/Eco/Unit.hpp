#pragma once

#include "Eco/Private/Config.hpp"

#include <type_traits>

namespace Eco {
inline namespace Eco_NS {

struct Unit {};

template<typename T>
using LiftUnit = std::conditional_t<std::is_void_v<T>, Unit, T>;

} // inline namespace Eco_NS
} // namespace Eco
