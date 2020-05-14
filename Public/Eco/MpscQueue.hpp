#pragma once

#include "Eco/Assert.hpp"
#include "Eco/Atomic.hpp"
#include "Eco/Link.hpp"
#include "Eco/Private/Config.hpp"

#include <concepts>
#include <type_traits>

namespace Eco {
// inline namespace Eco_NS {

using MpscQueueLink = Link<1>;

namespace Private::MpscQueue_ {

#define Eco_MPSCQ_HOOK(element) \
	(reinterpret_cast<Hook*>(static_cast<MpscQueueLink*>(element)))

#define Eco_MPSCQ_ELEM(hook) \
	(static_cast<T*>(reinterpret_cast<MpscQueueLink*>(hook)))

struct Hook : LinkBase
{
	Hook* next;
};

struct Core : LinkContainer
{
	// Singly linked list for producers to push into.
	// Ordered newest element first.
	atomic<Hook*> m_enqueue = nullptr;

	// Singly linked list for the consumer to pop from.
	// Ordered oldest element first.
	Hook* m_dequeue = nullptr;

	void Enqueue(Hook* hook);
	Hook* TryDequeue();
};

template<std::derived_from<MpscQueueLink> T>
class MpscQueue : Core
{
public:
	/// @return True if the queue is empty.
	/// @pre The invocation is externally synchronized and does not race with an invocation from another thread.
	bool IsEmpty() const
	{
		return m_dequeue == nullptr && m_enqueue.load(std::memory_order::relaxed) != nullptr;
	}


	/// @brief Enqueue an element into the queue.
	/// @param element Element to be enqueued.
	/// @pre @p element is not part of a container.
	void Enqueue(T* const element)
	{
		Core::Enqueue(Eco_MPSCQ_HOOK(element));
	}

	/// @brief Attempt to dequeue an element from the queue.
	/// @return The dequeued element, or nullptr if empty.
	/// @pre The invocation is externally synchronized and does not race with an invocation from another thread.
	T* TryDequeue()
	{
		return Eco_MPSCQ_ELEM(Core::TryDequeue());
	}
};

#undef Eco_MPSCQ_HOOK
#undef Eco_MPSCQ_ELEM

} // namespace Private::MpscQueue_

using Private::MpscQueue_::MpscQueue;

// } // inline namespace Eco_NS
} // namespace Eco
