#pragma once

#include <experimental/coroutine>

namespace Eco {

namespace stdcoro = std::experimental;

template<typename T = void>
struct Coro;

namespace Private {

template<typename T>
class CoroBase
{
	stdcoro::coroutine_handle<T> m_coro;

public:
	CoroBase()
	{
		m_coro = {};
	}

	CoroBase(stdcoro::coroutine_handle<T> coro)
	{
		m_coro = coro;
	}

	CoroBase(CoroBase&& src)
	{
		m_coro = std::exchange(src.m_coro, {});
	}

	CoroBase(const CoroBase&) = delete;

	CoroBase& operator=(CoroBase&& src)
	{
		if (m_coro) m_coro.destroy();
		m_coro = std::exchange(src.m_coro, {});
	}

	CoroBase& operator=(const CoroBase&) = delete;

	~CoroBase()
	{
		if (m_coro) m_coro.destroy();
	}

	stdcoro::coroutine_handle<T> Release()
	{
		return std::exchange(m_coro, {});
	}

	bool operator==(decltype(nullptr)) const
	{
		return !m_coro;
	}
	
	bool operator!=(decltype(nullptr)) const
	{
		return m_coro;
	}

private:
	friend struct Coro<T>;
};

} // namespace Private

template<typename T>
struct Coro : Private::CoroBase<T>
{
	using Private::CoroBase<T>::CoroBase;

	T& operator*() const
	{
		return Private::CoroBase<T>::m_coro.promise();
	}
	
	T* operator->() const
	{
		return &Private::CoroBase<T>::m_coro.promise();
	}
};

template<>
struct Coro<void> : Private::CoroBase<void>
{
	using Private::CoroBase<void>::CoroBase;

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

class CoroCallOnce
{
	Coro<> m_coro;

public:
	CoroCallOnce(Coro<> coro)
		: m_coro(std::move(coro))
	{
	}

	void operator()()
	{
		m_coro.Release().resume();
	}
};

} // namespace Eco
