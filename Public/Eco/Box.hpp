#pragma once

#include <concepts>

namespace Eco {

template<typename T>
class Box
{
	T m_value;

public:
	template<std::convertible_to<T> = T>
	Box(T&& value)
		: m_value(value)
	{
	}

	template<typename... Ts>
	explicit Box(Ts&&... args)
		requires std::constructible_from<T, Ts...>
		: m_value(static_cast<T&&>(args)...)
	{
	}

	T& operator*()
	{
		return m_value;
	}

	const T& operator*() const
	{
		return m_value;
	}

	T* operator->()
	{
		return &m_value;
	}

	const T* operator->() const
	{
		return &m_value;
	}
};

} // namespace Eco
