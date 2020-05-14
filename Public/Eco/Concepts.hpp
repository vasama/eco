#pragma once

#include "Eco/TypeTraits.hpp"

#include <concepts>

namespace Eco {

template<typename T1, typename T2>
concept not_same_as = !std::same_as<T1, T2>;

template<typename T, template<typename...> typename TTemplate>
concept specialization_of = is_specialization_of_v<T, TTemplate>;

} // namespace Eco
