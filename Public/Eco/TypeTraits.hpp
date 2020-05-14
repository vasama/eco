#pragma once

#include <type_traits>

namespace Eco {

template<typename T, template<typename...> typename TTemplate>
struct is_specialization_of : std::false_type {};

template<template<typename...> typename TTemplate, typename... TArgs>
struct is_specialization_of<TTemplate<TArgs...>, TTemplate> : std::true_type {};

template<typename T, template<typename...> typename TTemplate>
inline bool is_specialization_of_v = is_specialization_of<T, TTemplate>::value;

} // namespace Eco
