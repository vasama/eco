#pragma once

#include "Eco/Private/Config.hpp"
#include "Eco/Private/Std20.hpp"

#include <type_traits>

#include <cstdint>

namespace Eco {
inline namespace Eco_NS {

template<typename T, size_t TSize = 3>
class Function;

namespace _ {

template<typename TResult, typename... TParams>
struct FunctionTable
{
	TResult(*invoke)(void* object, TParams... args);
	void(*relocate)(void* object, void* buffer);
	void(*destroy)(void* object);
};

template<typename T, typename TResult, typename... TParams>
const FunctionTable<TResult, TParams...> FunctionTableInstance =
{
	[](void* object, TParams... args) -> TResult
	{
		return (*(T*)object)(static_cast<TParams&&>(args)...);
	},
	[](void* object, void* buffer) -> void
	{
		::new (buffer) T(static_cast<T&&>(*(T*)object));
		((T*)object)->~T();
	},
	[](void* object) -> void
	{
		((T*)object)->~T();
	}
};

template<typename T, size_t TSize, typename... TParams>
concept FunctionCallable = std::is_invocable_v<T, TParams...> &&
	sizeof(T) <= TSize * sizeof(uintptr_t) && alignof(T) <= alignof(uintptr_t);

} // namespace _

template<typename TResult, typename... TParams, size_t TSize>
class Function<TResult(TParams...), TSize>
{
	typedef _::FunctionTable<TResult, TParams...> TableType;

	const TableType* m_table;
	uintptr_t m_buffer[TSize];

public:
	Function()
	{
		m_table = nullptr;
	}

	Function(_::FunctionCallable<TSize, TParams...> auto&& callable)
	{
		typedef std::decay_t<decltype(callable)> T;
		m_table = &_::FunctionTableInstance<T, TResult, TParams...>;
		::new (m_buffer) T(static_cast<T&&>(callable));
	}

	Function(Function&& src)
	{
		m_table = src.m_table;
		src.m_table = nullptr;

		if (m_table != nullptr)
		{
			m_table->relocate(src.m_buffer, m_buffer);
		}
	}

	Function(const Function&) = delete;

	Function& operator=(Function&& src)
	{
		if (m_table != nullptr)
		{
			m_table->destroy(m_buffer);
		}
		
		m_table = src.m_table;
		src.m_table = nullptr;

		if (m_table != nullptr)
		{
			m_table->relocate(src.m_buffer, m_buffer);
		}
	}
	
	Function& operator=(const Function&) = delete;

	~Function()
	{
		if (m_table != nullptr)
		{
			m_table->destroy(m_buffer);
		}
	}

	bool operator==(decltype(nullptr))
	{
		return m_table == nullptr;
	}

	bool operator!=(decltype(nullptr))
	{
		return m_table != nullptr;
	}

	TResult operator()(std20::convertible_to<TParams> auto&&... args)
	{
		return m_table->invoke(m_buffer, static_cast<decltype(args)&&>(args)...);
	}
};

typedef Function<void()> Action;

} // inline namespace Eco_NS
} // namespace Eco
