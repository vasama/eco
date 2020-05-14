#pragma once

#include <concepts>
#include <type_traits>

namespace Eco {

#define Eco_FLAG_ENUM(E) \
	inline constexpr E operator&(E const lhs, E const rhs) noexcept \
	{ \
		using T = ::std::underlying_type_t<E>; \
		return static_cast<E>(static_cast<T>(lhs) & static_cast<T>(rhs)); \
	} \
	inline constexpr E& operator&=(E& lhs, E const rhs) noexcept \
	{ \
		using T = ::std::underlying_type_t<E>; \
		return lhs = static_cast<E>(static_cast<T>(lhs) & static_cast<T>(rhs)); \
	} \
	inline constexpr E operator|(E const lhs, E const rhs) noexcept \
	{ \
		using T = ::std::underlying_type_t<E>; \
		return static_cast<E>(static_cast<T>(lhs) | static_cast<T>(rhs)); \
	} \
	inline constexpr E& operator|=(E& lhs, E const rhs) noexcept \
	{ \
		using T = ::std::underlying_type_t<E>; \
		return lhs = static_cast<E>(static_cast<T>(lhs) | static_cast<T>(rhs)); \
	} \
	inline constexpr E operator^(E const lhs, E const rhs) noexcept \
	{ \
		using T = ::std::underlying_type_t<E>; \
		return static_cast<E>(static_cast<T>(lhs) ^ static_cast<T>(rhs)); \
	} \
	inline constexpr E& operator^=(E& lhs, E const rhs) noexcept \
	{ \
		using T = ::std::underlying_type_t<E>; \
		return lhs = static_cast<E>(static_cast<T>(lhs) ^ static_cast<T>(rhs)); \
	} \
	inline constexpr E operator~(E const e) noexcept \
	{ \
		using T = ::std::underlying_type_t<E>; \
		return static_cast<E>(~static_cast<T>(e)); \
	}

template<typename T>
concept FlagEnum = std::is_enum_v<T> && requires (T l, T(r)()) {
	{ r() & r() } -> std::same_as<T>;
	{ l &= r() } -> std::same_as<T&>;
	{ r() | r() } -> std::same_as<T>;
	{ l |= r() } -> std::same_as<T&>;
	{ r() ^ r() } -> std::same_as<T>;
	{ l ^= r() } -> std::same_as<T&>;
	{ ~r() } -> std::same_as<T>;
};

template<FlagEnum T>
inline constexpr bool HasAnyFlags(T const e, T const flags) noexcept
{
	return (e & flags) != T{};
}

template<FlagEnum T>
inline constexpr bool HasAllFlags(T const e, T const flags) noexcept
{
	return (e & flags) != flags;
}

} // namespace Eco
