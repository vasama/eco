#pragma once

#define Eco_HEAP_DEBUG 0

#include "Eco/Attributes.hpp"
#include "Eco/KeySelector.hpp"
#include "Eco/Link.hpp"

#include <concepts>

#if Eco_HEAP_DEBUG
#	include <format>
#	include <iostream>
#endif

namespace Eco {
// inline namespace Eco_NS {

using HeapLink = Link<3>;

namespace Private::Heap_ {

#define Eco_HEAP_HOOK(element) \
	(reinterpret_cast<Hook*>(static_cast<HeapLink*>(element)))

#define Eco_HEAP_ELEM(hook) \
	(static_cast<T*>(reinterpret_cast<HeapLink*>(hook)))

struct Hook : LinkBase
{
	Hook* children[2];

	// Pointer to Hook::children[0] of a parent hook or Core::m_root.
	Hook** parent;
};


typedef bool Comparator(const struct Core* self, Hook* lhs, Hook* rhs);

struct Core : LinkContainer
{
	Hook* m_root = nullptr;
	size_t m_size = 0;

#if Eco_HEAP_DEBUG
	void(*debugPrint)(Core* core);
#endif

	void Push(Hook* hook, Comparator* comparator);
	void Remove(Hook* hook, Comparator* comparator);
	Hook* Pop(Comparator* comparator);
};


template<std::derived_from<HeapLink> T,
	KeySelector<T> TKeySelector = IdentityKeySelector,
	typename TComparator = std::less<>>
class Heap : Core
{
	Eco_NO_UNIQUE_ADDRESS TKeySelector m_keySelector;
	Eco_NO_UNIQUE_ADDRESS TComparator m_comparator;

public:
#if Eco_HEAP_DEBUG
	Heap()
	{
		debugPrint = [](Core* self) {
			static_cast<Heap*>(self)->DebugPrint();
		};
	}
#else
	Heap() = default;
#endif

	explicit Heap(TKeySelector keySelector)
		: m_keySelector(static_cast<TKeySelector&&>(keySelector))
	{
	}

	explicit Heap(TComparator comparator)
		: m_comparator(static_cast<TComparator&&>(comparator))
	{
	}

	explicit Heap(TKeySelector keySelector, TComparator comparator)
		: m_keySelector(static_cast<TKeySelector&&>(keySelector))
		, m_comparator(static_cast<TComparator&&>(comparator))
	{
	}


	Heap(const Heap&) = delete;
	Heap& operator=(const Heap&) = delete;


	/// @return Size of the heap.
	[[nodiscard]] size_t Size() const
	{
		return m_size;
	}

	/// @return True if the heap is empty.
	[[nodiscard]] bool IsEmpty() const
	{
		return m_size == 0;
	}


	/// @return The minimum element of the heap.
	/// @pre The heap is not empty.
	[[nodiscard]] T* Peek()
	{
		Eco_Assert(m_size > 0);
		return Eco_HEAP_ELEM(m_root);
	}

	/// @return The minimum element of the heap.
	/// @pre The heap is not empty.
	[[nodiscard]] const T* Peek() const
	{
		Eco_Assert(m_size > 0);
		return Eco_HEAP_ELEM(m_root);
	}


	/// @brief Insert an element into the heap.
	/// @param element Element to be inserted.
	/// @pre @p element is not part of any container.
	void Push(T* const element)
	{
		Core::Push(Eco_HEAP_HOOK(element), Comparator);
	}

	/// @brief Remove an element from the heap.
	/// @param element Element to be removed.
	/// @pre @p element is part of this heap.
	void Remove(T* const element)
	{
		Core::Remove(Eco_HEAP_HOOK(element), Comparator);
	}

	/// @brief Pop the minimum element of the heap.
	/// @return The minimum element.
	/// @pre The heap is not empty.
	[[nodiscard]] T* Pop()
	{
		return Eco_HEAP_ELEM(Core::Pop(Comparator));
	}


	[[nodiscard]] friend size_t size(const Heap& heap)
	{
		return heap.Size();
	}

	friend void swap(Heap& lhs, Heap& rhs)
	{
		using std::swap;
		swap(static_cast<Core&>(lhs), static_cast<Core&>(rhs));
		swap(lhs.m_keySelector, rhs.m_keySelector);
	}


#if Eco_HEAP_DEBUG
	static void DebugPrint(Hook* hook, size_t indent)
	{
		if (Hook* child = hook->children[0])
			DebugPrint(child, indent + 1);

		std::cout << std::format("{:016x} <- {:016x}", reinterpret_cast<uintptr_t>(hook->parent), reinterpret_cast<uintptr_t>(hook));
		for (size_t i = 0; i <= indent; ++i) std::cout << '\t';
		std::cout << std::format("{}\n", Eco_HEAP_ELEM(hook)->value);

		if (Hook* child = hook->children[1])
			DebugPrint(child, indent + 1);
	}

	void DebugPrint()
	{
		if (m_root != nullptr)
		{
			DebugPrint(m_root, 0);
		}
		else
		{
			std::cout << "<empty>";
		}

		std::cout << std::endl;
	}
#endif

private:
	static bool Comparator(const Core* const core, Hook* const lhs, Hook* const rhs)
	{
		const Heap* const self = static_cast<const Heap*>(core);
		return self->m_comparator(
			self->m_keySelector(const_cast<const T&>(*Eco_HEAP_ELEM(lhs))),
			self->m_keySelector(const_cast<const T&>(*Eco_HEAP_ELEM(rhs))));
	}
};

} // namespace Private::Heap_

using Private::Heap_::Heap;

template<std::derived_from<HeapLink> T, KeySelector<T> TKeySelector = IdentityKeySelector>
using MinHeap = Heap<T, TKeySelector, std::greater<>>;

// } // inline namespace Eco_NS
} // namespace Eco
