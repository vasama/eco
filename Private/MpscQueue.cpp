#include "Eco/MpscQueue.hpp"

#include <utility>

using namespace Eco;
using namespace Private::MpscQueue_;

static Hook* ReverseList(Hook* head)
{
	Hook* prev = nullptr;

	while (head != nullptr)
	{
		head = std::exchange(head->next, std::exchange(prev, head));
	}

	return prev;
}

void Core::Enqueue(Hook* const hook)
{
	LinkInsert(*hook, *this);

	Hook* head = m_enqueue.load(std::memory_order::acquire);
	do
	{
		hook->next = head;
	} while (!m_enqueue.compare_exchange_weak(head, hook,
		std::memory_order::release, std::memory_order::acquire));
}

Hook* Core::TryDequeue()
{
	// Pop from the dequeue list if it is not empty.
	if (Hook* const head = m_dequeue)
	{
		m_dequeue = head->next;

		LinkRemove(*head, *this);

		return head;
	}

	// Test the enqueue list before exchanging.
	if (m_enqueue.load(std::memory_order::acquire) != nullptr)
	{
		// Exchange the enqueue list. It is hence wholly owned by this thread.
		if (Hook* head = m_enqueue.exchange(nullptr, std::memory_order::acq_rel))
		{
			// The enqueue list is ordered latest first. Reverse it.
			head = ReverseList(head);

			// The first element of the new dequeue list is returned.
			// The rest of the list is stored for later.
			m_dequeue = head->next;

			LinkRemove(*head, *this);

			return head;
		}
	}

	return nullptr;
}
