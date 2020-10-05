#pragma once

#include "Eco/Private/Config.hpp"
#include "Eco/Private/PriorityTag.hpp"
#include "Eco/Private/Std20.hpp"

#include <type_traits>

namespace Eco {
inline namespace Eco_NS {

template<typename T>
concept Awaiter =
	requires(T awaiter, std20::coroutine_handle<> coro)
	{
		awaiter.await_ready();
		awaiter.await_suspend(coro);
		awaiter.await_resume();
	};

template<typename T>
concept Awaitable =
	requires(T awaitable) { awaitable.operator co_await(); } ||
	requires(T awaitable) { operator co_await(awaitable); } ||
	Awaiter<T>;

namespace _ {

template<Awaiter T>
Eco_FORCEINLINE T&& GetAwaiter(T&& awaiter, PriorityTag<0>)
{
	return static_cast<T&&>(awaiter);
}

template<typename T>
Eco_FORCEINLINE decltype(auto) GetAwaiter(T&& awaitable, PriorityTag<1>)
	requires(requires{ static_cast<T&&>(awaitable).operator co_await(); })
{
	return static_cast<T&&>(awaitable).operator co_await();
}

template<typename T>
Eco_FORCEINLINE decltype(auto) GetAwaiter(T&& awaitable, PriorityTag<2>)
	requires(requires{ operator co_await(static_cast<T&&>(awaitable)); })
{
	return operator co_await(static_cast<T&&>(awaitable));
}

} // namespace _

template<Awaitable T>
Eco_FORCEINLINE decltype(auto) GetAwaiter(T&& awaitable)
{
	return _::GetAwaiter(static_cast<T&&>(awaitable), _::PriorityTag<2>{});
}

template<Awaitable T>
using AwaiterType = std::decay_t<decltype(GetAwaiter(std::declval<T>()))>;


template<typename T = void>
struct Coro;

namespace _ {

template<typename T>
class CoroBase
{
public:
	typedef std20::coroutine_handle<T> HandleType;

private:
	HandleType m_handle;

public:
	CoroBase()
	{
		m_handle = {};
	}

	CoroBase(HandleType coro)
	{
		m_handle = coro;
	}

	CoroBase(CoroBase&& src)
		: m_handle(src.m_handle)
	{
		src.m_handle = HandleType{};
	}

	CoroBase(const CoroBase&) = delete;

	CoroBase& operator=(CoroBase&& src)
	{
		if (m_handle) m_handle.destroy();
		m_handle = src.m_handle;
		src.m_handle = HandleType{};
		return *this;
	}

	CoroBase& operator=(const CoroBase&) = delete;

	~CoroBase()
	{
		if (m_handle) m_handle.destroy();
	}

	HandleType Release()
	{
		HandleType handle = m_handle;
		m_handle = HandleType{};
		return handle;
	}

	bool operator==(decltype(nullptr)) const
	{
		return !m_handle;
	}
	
	bool operator!=(decltype(nullptr)) const
	{
		return m_handle;
	}

private:
	friend struct Coro<T>;
};

} // namespace _

template<typename T>
struct Coro : _::CoroBase<T>
{
	using _::CoroBase<T>::CoroBase;

	T& operator*() const
	{
		return _::CoroBase<T>::m_handle.promise();
	}
	
	T* operator->() const
	{
		return &_::CoroBase<T>::m_handle.promise();
	}
};

template<>
struct Coro<void> : _::CoroBase<void>
{
	using _::CoroBase<void>::CoroBase;

	template<typename TIn>
	Coro(std20::coroutine_handle<TIn> handle)
		: CoroBase<void>(handle)
	{
	}

	template<typename TIn>
	Coro(Coro<TIn>&& src)
		: CoroBase<void>(src.Release())
	{
	}

	template<typename TIn>
	Coro& operator=(Coro<TIn>&& src)
	{
		CoroBase<void>::operator=(src.Release());
		return *this;
	}
};

class CoroCallable
{
	Coro<> m_handle;

public:
	template<typename T>
	CoroCallable(std20::coroutine_handle<T> coro)
		: m_handle(coro)
	{
	}

	template<typename T>
	CoroCallable(Coro<T>&& coro)
		: m_handle(coro.Release())
	{
	}

	void operator()()
	{
		m_handle.Release().resume();
	}
};

} // inline namespace Eco_NS
} // namespace Eco

