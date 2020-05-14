#pragma once

#include <type_traits>
#include <utility>

namespace Eco {

template<typename T, auto Default = T{}>
	requires std::is_trivially_copyable_v<T>
struct Linear
{
	T Value = Default;

	Linear() = default;

	constexpr Linear(T const value)
		: Value(value)
	{
	}

	explicit Linear(std::in_place_t, auto&&... args)
		: Value(static_cast<decltype(args)&&>(args)...)
	{
	}

	constexpr Linear(Linear&& src) noexcept
		: Value(std::exchange(src.Value, Default))
	{
	}

	constexpr Linear& operator=(T const value) noexcept
	{
		Value = value;
		return *this;
	}

	constexpr Linear& operator=(Linear&& src) noexcept
	{
		Value = std::exchange(src.Value, Default);
		return *this;
	}

	constexpr T& operator*() noexcept
	{
		return Value;
	}

	constexpr const T& operator*() const noexcept
	{
		return Value;
	}

	constexpr T* operator->() noexcept
	{
		return &Value;
	}

	constexpr const T* operator->() const noexcept
	{
		return &Value;
	}
};

} // namespace Eco
