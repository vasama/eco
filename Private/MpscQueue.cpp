#include "Eco/MpscQueue.hpp"

using namespace Eco;

void Eco::Eco_NS::_::MpscQueue::Enqueue(SingleLink<void>* object)
{
	SingleLink<void>* head = m_enqueue.load(std::memory_order_acquire);
	do
	{
		object->m_next = head;
	} while (!m_enqueue.compare_exchange_weak(head, object,
		std::memory_order_release, std::memory_order_acquire));
}

SingleLink<void>* Eco::Eco_NS::_::MpscQueue::TryDequeue()
{
	SingleLink<void>* object = m_dequeue;

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
	if (SingleLink<void>* next = object->m_next)
	{
		object->m_next = nullptr;
		while (SingleLink<void>* next2 = next->m_next)
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
