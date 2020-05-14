#pragma once

#include "Eco/ForwardList.hpp"
#include "Eco/Std20.hpp"

#include <atomic>
#include <type_traits>

#include <assert.h>

namespace Eco {

template<typename T>
	requires std::is_void_v<T> || std::is_base_of_v<ForwardListObject<T>, T>
class MpscListQueue;

template<>
class MpscListQueue<void>
{
	std::atomic<ForwardListObject<void>*> m_enqueue = nullptr;
	ForwardListObject<void>* m_dequeue = nullptr;

public:
	void Enqueue(ForwardListObject<void>* object);
	ForwardListObject<void>* TryDequeue();
};

template<typename T>
	requires std::is_void_v<T> || std::is_base_of_v<ForwardListObject<T>, T>
class MpscListQueue : MpscListQueue<void>
{
public:
	void Enqueue(T* object)
	{
		MpscListQueue<void>::Enqueue(static_cast<ForwardListObject<T>*>(object));
	}

	T* TryDequeue()
	{
		return static_cast<T*>(MpscListQueue<void>::TryDequeue());
	}
};

} // namespace Eco
