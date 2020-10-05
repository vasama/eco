#pragma once

#include "Eco/Assert.hpp"
#include "Eco/Coroutine.hpp"
#include "Eco/Lock.hpp"
#include "Eco/Private/Config.hpp"
#include "Eco/Private/Std20.hpp"

#include <atomic>

namespace Eco {
inline namespace Eco_NS {

class Mutex
{
	class ScopeAwaiter;

	template<typename>
	class Awaitable;

	class Awaiter
	{
		Mutex& m_mutex;
		Coro<> m_coro;
		Awaiter* m_next;

	public:
		Awaiter(const Awaiter&) = delete;
		Awaiter& operator=(const Awaiter&) = delete;

		bool await_ready() const noexcept
		{
			return m_mutex.TryLock();
		}

		bool await_suspend(std20::coroutine_handle<> continuation) noexcept
		{
			m_coro = continuation;
			return m_mutex.LockInternal(this);
		}

		void await_resume() const noexcept
		{
		}

	private:
		Awaiter(Mutex& mutex)
			: m_mutex(mutex)
		{
		}

		friend class Mutex;
		friend class Awaitable<Awaiter>;
	};

	class ScopeAwaiter : public Awaiter
	{
		using Awaiter::Awaiter;

	public:
		Lock<Mutex> await_resume() const noexcept
		{
			return Lock<Mutex>(m_mutex, AdoptLock);
		}

	private:
		friend class Awaitable<ScopeAwaiter>;
	};

	template<typename TAwaiter>
	class Awaitable
	{
		Mutex& m_mutex;

	public:
		

	private:
		Awaitable(Mutex& mutex)
			: m_mutex(mutex)
		{
		}

		friend class Mutex;
	};

	std::atomic<void*> m_state;
	Awaiter* m_queue;

public:
	Mutex() noexcept
		: m_state(UnlockedState()), m_queue(nullptr)
	{
	}

	Mutex(const Mutex&) = delete;
	Mutex& operator=(const Mutex&) = delete;

#if Eco_CONFIG_ASSERT
	~Mutex()
	{
		Eco_Assert(DestructorCheck());
	}
#endif

	[[nodiscard]] bool TryLock() noexcept
	{
		void* oldState = UnlockedState();
		return m_state.compare_exchange_strong(oldState, nullptr,
			std::memory_order_acquire, std::memory_order_relaxed);
	}

	[[nodiscard]] Awaitable<Awaiter> LockAsync() noexcept
	{
		return Awaitable<Awaiter>(*this);
	}

	[[nodiscard]] Awaitable<ScopeAwaiter> LockScopeAsync() noexcept
	{
		return Awaitable<ScopeAwaiter>(*this);
	}

	void Unlock() noexcept;

private:
	void* UnlockedState() const noexcept
	{
		return const_cast<Mutex*>(this);
	}

	bool LockInternal(Awaiter* awaiter) noexcept;

#if Eco_CONFIG_ASSERT
	bool DestructorCheck() const noexcept;
#endif
};

} // inline namespace Eco_NS
} // namespace Eco
