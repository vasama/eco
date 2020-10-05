#include "Eco/Mutex.hpp"

void Eco::Eco_NS::Mutex::Unlock() noexcept
{
	Eco_Assert(m_state.load(std::memory_order_relaxed) != UnlockedState());

	Awaiter* queue = m_queue;

	if (queue == nullptr)
	{
		void* state = m_state.load(std::memory_order_relaxed);

		if (state == nullptr && m_state.compare_exchange_strong(state,
			UnlockedState(), std::memory_order_release, std::memory_order_relaxed))
		{
			return;
		}

		state = m_state.exchange(nullptr, std::memory_order_acquire);
		Eco_Assert(state != nullptr && state != UnlockedState());

		Awaiter* newQueue = static_cast<Awaiter*>(state);

		do
		{
			Awaiter* next = newQueue->m_next;
			newQueue->m_next = queue;
			queue = newQueue;
			newQueue = next;
		} while (newQueue != nullptr);
	}

	m_queue = queue->m_next;
	queue->m_coro.Release().resume();
}

bool Eco::Eco_NS::Mutex::LockInternal(Awaiter* awaiter) noexcept
{
	void* oldState = m_state.load(std::memory_order_relaxed);

	while (true)
	{
		if (oldState == UnlockedState())
		{
			void* newState = nullptr;
			if (m_state.compare_exchange_weak(oldState, newState,
				std::memory_order_acquire, std::memory_order_relaxed))
			{
				return false;
			}
		}
		else
		{
			void* newState = awaiter;
			awaiter->m_next = static_cast<Awaiter*>(oldState);
			if (m_state.compare_exchange_weak(oldState, newState,
				std::memory_order_acquire, std::memory_order_relaxed))
			{
				return true;
			}
		}
	}
}

#if Eco_CONFIG_ASSERT
bool Eco::Eco_NS::Mutex::DestructorCheck() const noexcept
{
	void* state = m_state.load(std::memory_order_relaxed);
	return (state == UnlockedState() || state == nullptr) && m_queue == nullptr;
}
#endif
