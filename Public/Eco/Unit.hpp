#pragma once

#include <type_traits>

namespace Eco {

struct Unit {};

template<typename T>
using LiftUnit = std::conditional_t<std::is_void_v<T>, Unit, T>;

} // namespace Eco
