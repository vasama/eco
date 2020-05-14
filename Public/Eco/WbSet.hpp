#pragma once

#define Eco_WB_DEBUG 0

#include "Eco/Attributes.hpp"
#include "Eco/InsertResult.hpp"
#include "Eco/KeySelector.hpp"
#include "Eco/Linear.hpp"
#include "Eco/Link.hpp"
#include "Eco/List.hpp"
#include "Eco/TaggedPointer.hpp"

#include <concepts>
#include <ranges>

#if Eco_WB_DEBUG
#	include <format>
#	include <iostream>
#endif

namespace Eco {

using WbSetLink = Link<4>;

namespace Private::WbSet_ {

#define Eco_WB_HOOK(elem, ...) \
	(reinterpret_cast<Hook __VA_ARGS__*>(static_cast<WbSetLink __VA_ARGS__*>(elem)))

#define Eco_WB_ELEM(hook, ...) \
	(static_cast<T __VA_ARGS__*>(reinterpret_cast<WbSetLink __VA_ARGS__*>(hook)))

#define Eco_WB_HOOK_FROM_CHILDREN(children) \
	static_cast<Hook*>(reinterpret_cast<HookContent*>(children))

template<typename T>
using Ptr = IncompleteTaggedPointer<T, uintptr_t, 1>;

struct Hook;

struct HookContent
{
	Hook* children[2];

	// Pointer to children[0] of a parent hook or Core::m_root;
	Hook** parent;

	// Weight of the subtree rooted at this node, including this node.
	uintptr_t weight;
};

struct Hook : LinkBase, HookContent {};


struct Core : LinkContainer
{
	Linear<Hook*> m_root;

#if Eco_WB_DEBUG
	void(*debugPrint)(Core* core, bool nums);
#endif

	struct FindResult
	{
		Hook* hook;
		Ptr<Hook*> parent;
	};

	Hook* Select(size_t rank) const;
	size_t Rank(const Hook* hook) const;

	void Insert(Hook* hook, Ptr<Hook*> parentAndSide);
	void Remove(Hook* hook);
	void Clear();
	List_::Hook* Flatten();

	friend void swap(Core& lhs, Core& rhs) noexcept;
};

Hook** IteratorBegin(Hook** root);
Hook** IteratorAdvance(Hook** children, bool l);


class IteratorCore
{
	Hook** m_children;

public:
	IteratorCore() = default;

	IteratorCore(Hook** const children)
		: m_children(children)
	{
	}

	bool operator==(const IteratorCore&) const = default;

private:
	template<typename T>
	friend struct Iterator;
};

template<typename T>
struct Iterator : IteratorCore
{
	using difference_type = ptrdiff_t;
	using value_type = T;
	using pointer = T*;
	using reference = T&;


	using IteratorCore::IteratorCore;


	[[nodiscard]] T& operator*() const
	{
		return *Eco_WB_ELEM(Eco_WB_HOOK_FROM_CHILDREN(m_children));
	}

	[[nodiscard]] T* operator->() const
	{
		return Eco_WB_ELEM(Eco_WB_HOOK_FROM_CHILDREN(m_children));
	}


	Iterator& operator++()
	{
		m_children = IteratorAdvance(m_children, 0);
		return *this;
	}

	[[nodiscard]] Iterator operator++(int)
	{
		auto it = *this;
		m_children = IteratorAdvance(m_children, 0);
		return it;
	}

	Iterator& operator--()
	{
		m_children = IteratorAdvance(m_children, 1);
		return *this;
	}

	[[nodiscard]] Iterator operator--(int)
	{
		auto it = *this;
		m_children = IteratorAdvance(m_children, 1);
		return it;
	}


	[[nodiscard]] bool operator==(const Iterator&) const = default;
};


template<typename T>
struct WbSetChildren
{
	T* Children[2];

	T* operator[](size_t const index) const
	{
		Eco_Assert(index < 2);
		return Children[index];
	}
};

template<std::derived_from<WbSetLink> T,
	KeySelector<T> TKeySelector = IdentityKeySelector,
	typename TComparator = std::compare_three_way>
class WbSet : Core
{
	using KeyType = decltype(std::declval<const TKeySelector&>()(std::declval<const T&>()));
	using RootType = Hook*;

	Eco_NO_UNIQUE_ADDRESS TKeySelector m_keySelector;
	Eco_NO_UNIQUE_ADDRESS TComparator m_comparator;

public:
#if Eco_WB_DEBUG
	bool m_debugPrintEnable = false;
#endif

	using ElementType = T;

	using       iterator = Iterator<      T>;
	using const_iterator = Iterator<const T>;

	using InsertResult = Eco::InsertResult<T>;


#if Eco_WB_DEBUG
	WbSet()
	{
		debugPrint = [](Core* self, bool nums) {
			static_cast<WbSet*>(self)->DebugPrint(nums);
		};
	}
#else
	WbSet() = default;
#endif

	explicit WbSet(TKeySelector keySelector)
		: m_keySelector(static_cast<TKeySelector&&>(keySelector))
	{
	}

	explicit WbSet(TComparator comparator)
		: m_comparator(static_cast<TComparator&&>(comparator))
	{
	}

	explicit WbSet(TKeySelector keySelector, TComparator comparator)
		: m_keySelector(static_cast<TKeySelector&&>(keySelector))
		, m_comparator(static_cast<TComparator&&>(comparator))
	{
	}

	WbSet& operator=(WbSet&& src) noexcept
	{
		if (m_root.Value != nullptr)
			Core::Clear();
		Core::operator=(static_cast<Core&&>(src));
		return *this;
	}

	~WbSet()
	{
		if (m_root.Value != nullptr)
			Core::Clear();
	}


	[[nodiscard]] size_t Size() const
	{
		return m_root.Value != nullptr ? m_root.Value->weight : 0;
	}

	[[nodiscard]] bool IsEmpty() const
	{
		return m_root.Value == nullptr;
	}


	[[nodiscard]] T* Root()
	{
		Eco_Assert(m_root.Value != nullptr);
		return Eco_WB_ELEM(m_root.Value);
	}

	[[nodiscard]] const T* Root() const
	{
		Eco_Assert(m_root.Value != nullptr);
		return Eco_WB_ELEM(m_root.Value);
	}


	[[nodiscard]] size_t Weight(const T* const element) const
	{
		LinkCheck(*element, *this);
		return Eco_WB_HOOK(element, const)->weight;
	}

	[[nodiscard]] WbSetChildren<T> Children(const T* const element)
	{
		LinkCheck(*element, *this);
		const Hook* const hook = Eco_WB_HOOK(element, const);
		return { Eco_WB_ELEM(hook->children[0]), Eco_WB_ELEM(hook->children[1]) };
	}

	[[nodiscard]] WbSetChildren<const T> Children(const T* const element) const
	{
		LinkCheck(*element, *this);
		const Hook* const hook = Eco_WB_HOOK(element, const);
		return { Eco_WB_ELEM(hook->children[0]), Eco_WB_ELEM(hook->children[1]) };
	}


	[[nodiscard]] T* Select(size_t const rank)
	{
		return Eco_WB_ELEM(Core::Select(rank));
	}

	[[nodiscard]] const T* Select(size_t const rank) const
	{
		return Eco_WB_ELEM(Core::Select(rank));
	}

	[[nodiscard]] size_t Rank(const T* const element) const
	{
		return Core::Rank(Eco_WB_HOOK(element));
	}


	[[nodiscard]] T* Find(const KeyType& key)
	{
		return Eco_WB_ELEM(FindInternal(key).hook);
	}

	[[nodiscard]] const T* Find(const KeyType& key) const
	{
		return Eco_WB_ELEM(FindInternal(key).hook);
	}

	template<typename TKey>
	[[nodiscard]] T* FindEquivalent(const TKey& key)
		requires (requires (const KeyType& containerKey) { m_comparator(key, containerKey); })
	{
		return Eco_WB_ELEM(FindInternal(key).hook);
	}

	template<typename TKey>
	[[nodiscard]] const T* FindEquivalent(const TKey& key) const
		requires (requires (const KeyType& containerKey) { m_comparator(key, containerKey); })
	{
		return Eco_WB_ELEM(FindInternal(key).hook);
	}


	InsertResult Insert(T* const element)
	{
		auto const r = FindInternal(m_keySelector(*element));
		if (r.hook != nullptr) return { Eco_WB_ELEM(r.hook), false };
		Core::Insert(Eco_WB_HOOK(element), r.parent);
		return { element, true };
	}

	void Remove(T* const element)
	{
		Core::Remove(Eco_WB_HOOK(element));
	}

	/// @brief Remove all elements from the tree.
	using Core::Clear;

	/// @brief Flatten the tree into a linked list using an in-order traversal.
	[[nodiscard]] List<T> Flatten()
	{
		size_t const size = Size();
		return List<T>(static_cast<LinkContainer&&>(*this), Core::Flatten(), size);
	}


	[[nodiscard]] iterator MakeIterator(T* const element)
	{
		LinkCheck(*element, *this);
		return iterator(Eco_WB_HOOK(element));
	}

	[[nodiscard]] const_iterator MakeIterator(const T* const element) const
	{
		LinkCheck(*element, *this);
		return const_iterator(Eco_WB_HOOK(const_cast<T*>(element)));
	}


	[[nodiscard]] iterator begin()
	{
		return iterator(IteratorBegin(&m_root.Value));
	}
	
	[[nodiscard]] const_iterator begin() const
	{
		return const_iterator(IteratorBegin(const_cast<RootType*>(&m_root.Value)));
	}

	[[nodiscard]] iterator end()
	{
		return iterator(&m_root.Value);
	}

	[[nodiscard]] const_iterator end() const
	{
		return const_iterator(const_cast<RootType*>(&m_root.Value));
	}


	[[nodiscard]] friend size_t size(const WbSet& set)
	{
		return set.Size();
	}

	friend void swap(WbSet& lhs, WbSet& rhs) noexcept
	{
		using std::swap;
		swap(static_cast<Core&>(lhs), static_cast<Core&>(rhs));
		swap(lhs.m_keySelector, rhs.m_keySelector);
		swap(lhs.m_comparator, rhs.m_comparator);
	}


#if Eco_WB_DEBUG
	int DebugPrint(Hook* hook, size_t indent, int num)
	{
		if (Hook* const child = hook->children[1])
			num = DebugPrint(child, indent + 1, num);

		std::cout << std::format("{:016x} ", reinterpret_cast<uintptr_t>(hook));

		for (size_t i = 0; i < indent; ++i) std::cout << '\t';
		std::cout << std::format("{: 2} {{{}}}\n", hook->weight, num < 0 ? Eco_WB_ELEM(hook)->value : num++);

		if (Hook* const child = hook->children[0])
			num = DebugPrint(child, indent + 1, num);

		return num;
	}

	void DebugPrint(bool nums = false)
	{
		if (!m_debugPrintEnable) return;

		if (m_root.Value != nullptr)
		{
			DebugPrint(m_root.Value, 0, -!nums);
		}
		else
		{
			std::cout << "<empty>";
		}

		std::cout << std::endl;
	}
#endif

private:
	template<typename TKey>
	FindResult FindInternal(const TKey& key) const
	{
		Hook** parent = const_cast<Hook**>(&m_root.Value);
		bool l = 0;

		while (parent[l] != nullptr)
		{
			Hook* const child = parent[l];

			auto const ordering = m_comparator(key, m_keySelector(*Eco_WB_ELEM(child)));
			if (ordering == 0) return { child, { parent, l } };

			parent = child->children;
			l = ordering > 0;
		}

		return FindResult{ nullptr, { parent, l } };
	}
};

#undef Eco_WB_HOOK
#undef Eco_WB_ELEM

} // namespace Private::WbSet_

using Private::WbSet_::WbSet;
using Private::WbSet_::WbSetChildren;

} // namespace Eco
