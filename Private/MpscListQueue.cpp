#include "Eco/MpscListQueue.hpp"

using namespace Eco;

void Eco::MpscListQueue<void>::Enqueue(ForwardListObject<void>* object)
{
	assert(object->m_next == nullptr);

	ForwardListObject<void>* head = m_enqueue.load(std::memory_order_acquire);
	do {
		object->m_next = head;
	} while (!m_enqueue.compare_exchange_weak(head, object,
		std::memory_order_release, std::memory_order_acquire));
}

ForwardListObject<void>* Eco::MpscListQueue<void>::TryDequeue()
{
	ForwardListObject<void>* object = m_dequeue;

	if (object != nullptr)
	{
		object->m_next = nullptr;
		m_dequeue = object->m_next;
		return object;
	}

	if (m_enqueue.load(std::memory_order_relaxed) == nullptr)
	{
		return nullptr;
	}

	object = m_enqueue.exchange(nullptr, std::memory_order_acq_rel);
	if (ForwardListObject<void>* next = object->m_next)
	{
		object->m_next = nullptr;
		while (ForwardListObject<void>* next2 = next->m_next)
		{
			next->m_next = object;
			object = next;
			next = next2;
		}

		m_dequeue = object;
		object = next;
	}
	object->m_next = nullptr;

	return object;
}
