#pragma once

#include "Eco/Assert.hpp"
#include "Eco/Link.hpp"
#include "Eco/Private/Config.hpp"

#include <atomic>
#include <type_traits>

namespace Eco {
inline namespace Eco_NS {

namespace _ {

class MpscQueue
{
	std::atomic<SingleLink<void>*> m_enqueue = nullptr;
	SingleLink<void>* m_dequeue = nullptr;

public:
	bool IsEmpty() const
	{
		return m_enqueue.load(std::memory_order_relaxed) == nullptr && m_dequeue == nullptr;
	}

protected:
	void Enqueue(SingleLink<void>* object);
	SingleLink<void>* TryDequeue();
};

} // namespace _

template<typename T>
	requires std::is_base_of_v<SingleLink<T>, T>
class MpscQueue : public _::MpscQueue
{
public:
	void Enqueue(T* object)
	{
		_::MpscQueue::Enqueue(reinterpret_cast<SingleLink<void>*>(static_cast<SingleLink<T>*>(object)));
	}

	T* TryDequeue()
	{
		return static_cast<T*>(reinterpret_cast<SingleLink<T>*>(_::MpscQueue::TryDequeue()));
	}
};

} // inline namespace Eco_NS
} // namespace Eco
