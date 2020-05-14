#pragma once

#include <type_traits>

namespace std {

template<typename T, typename... TArgs>
concept invocable = std::is_invocable_v<T, TArgs...>;

template<typename T, typename TIn>
concept convertible_to = std::is_convertible_v<TIn, T>;

template<typename T, typename TBase>
concept derived_from = std::is_base_of_v<TBase, T>;

} // namespace std
