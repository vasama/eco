#pragma once

#define Eco_AVL_DEBUG 0

#include "Eco/Attributes.hpp"
#include "Eco/InsertResult.hpp"
#include "Eco/KeySelector.hpp"
#include "Eco/Linear.hpp"
#include "Eco/Link.hpp"
#include "Eco/List.hpp"
#include "Eco/TaggedPointer.hpp"

#include <bit>
#include <concepts>
#include <ranges>

#if Eco_AVL_DEBUG
#	include <format>
#	include <iostream>
#endif

namespace Eco {
// inline namespace Eco_NS {

using AvlSetLink = Link<3>;

namespace Private::AvlSet_ {

#define Eco_AVL_HOOK(element) \
	(reinterpret_cast<Hook*>(static_cast<AvlSetLink*>(element)))

#define Eco_AVL_ELEM(hook) \
	(static_cast<T*>(reinterpret_cast<AvlSetLink*>(hook)))

#define Eco_AVL_HOOK_FROM_CHILDREN(children) \
	static_cast<Hook*>(reinterpret_cast<HookContent*>(children))

template<typename T>
using Ptr = IncompleteTaggedPointer<T, uintptr_t, 1>;

struct Hook;

struct HookContent
{
	// Child pointers with the low tag bit indicating +1 subtree height.
	Ptr<Hook> children[2];

	// Pointer to Hook::children[0] of a parent hook or Core::m_root.
	Ptr<Hook>* parent;
};

struct Hook : LinkBase, HookContent {};


struct Core : LinkContainer
{
	Linear<Ptr<Hook>, nullptr> m_root;
	Linear<size_t> m_size;

#if Eco_AVL_DEBUG
	void(*debugPrint)(Core* core, bool nums);
#endif

	struct FindResult
	{
		Hook* hook;
		Ptr<const Ptr<Hook>> parent;
	};

	void Insert(Hook* hook, Ptr<Ptr<Hook>> parentAndSide);
	void Remove(Hook* hook);
	void Clear();
	List_::Hook* Flatten();

	friend void swap(Core& lhs, Core& rhs) noexcept
	{
		using std::swap;
		swap(static_cast<LinkContainer&>(lhs), static_cast<LinkContainer&>(rhs));
		swap(lhs.m_root, rhs.m_root);
		swap(lhs.m_size, rhs.m_size);
	}
};

Ptr<Hook>* IteratorBegin(Ptr<Hook>* root);
Ptr<Hook>* IteratorAdvance(Ptr<Hook>* children, uintptr_t l);


class IteratorCore
{
	Ptr<Hook>* m_children;

public:
	IteratorCore() = default;

	IteratorCore(Ptr<Hook>* const children)
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

#if 0
	Iterator(Iterator<std::remove_const_t<T>> iterator)
		requires std::is_const_v<T>
		: IteratorCore(static_cast<IteratorCore>(iterator))
	{
	}
#endif


	[[nodiscard]] T& operator*() const
	{
		return *Eco_AVL_ELEM(Eco_AVL_HOOK_FROM_CHILDREN(m_children));
	}

	[[nodiscard]] T* operator->() const
	{
		return Eco_AVL_ELEM(Eco_AVL_HOOK_FROM_CHILDREN(m_children));
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


template<std::derived_from<AvlSetLink> T,
	KeySelector<T> TKeySelector = IdentityKeySelector,
	typename TComparator = std::compare_three_way>
class AvlSet : Core
{
	using KeyType = decltype(std::declval<const TKeySelector&>()(std::declval<const T&>()));
	using RootType = Ptr<Hook>;

	Eco_NO_UNIQUE_ADDRESS TKeySelector m_keySelector;
	Eco_NO_UNIQUE_ADDRESS TComparator m_comparator;

public:
#if Eco_AVL_DEBUG
	bool m_debugPrintEnable = false;
#endif

	using ElementType = T;

	using       iterator = Iterator<      T>;
	using const_iterator = Iterator<const T>;

	using InsertResult = Eco::InsertResult<T>;


#if Eco_AVL_DEBUG
	AvlSet()
	{
		debugPrint = [](Core* self, bool nums) {
			static_cast<AvlSet*>(self)->DebugPrint(nums);
		};
	}
#else
	AvlSet() = default;
#endif

	explicit AvlSet(TKeySelector keySelector)
		: m_keySelector(static_cast<TKeySelector&&>(keySelector))
	{
	}

	explicit AvlSet(TComparator comparator)
		: m_comparator(static_cast<TComparator&&>(comparator))
	{
	}

	explicit AvlSet(TKeySelector keySelector, TComparator comparator)
		: m_keySelector(static_cast<TKeySelector&&>(keySelector))
		, m_comparator(static_cast<TComparator&&>(comparator))
	{
	}

	AvlSet& operator=(AvlSet&& src) noexcept
	{
		if (!m_root->IsZero())
			Core::Clear();
		Core::operator=(static_cast<Core&&>(src));
		return *this;
	}

	~AvlSet()
	{
		if (!m_root->IsZero())
			Core::Clear();
	}


	/// @return Size of the set.
	[[nodiscard]] size_t Size() const
	{
		return m_size.Value;
	}

	/// @return True if the set is empty.
	[[nodiscard]] bool IsEmpty() const
	{
		return m_size.Value == 0;
	}


	/// @brief Find element by homogeneous key.
	/// @param key Lookup key.
	/// @return Pointer to element, or null if not found.
	[[nodiscard]] T* Find(const KeyType& key)
	{
		return Eco_AVL_ELEM(FindInternal(key).hook);
	}

	/// @brief Find element by homogeneous key.
	/// @param key Lookup key.
	/// @return Pointer to element, or null if not found.
	[[nodiscard]] const T* Find(const KeyType& key) const
	{
		return Eco_AVL_ELEM(FindInternal(key).hook);
	}

	/// @brief Find element by heterogeneous key.
	/// @param key Lookup key.
	/// @return Pointer to element or null.
	template<typename TKey>
	[[nodiscard]] T* FindEquivalent(const TKey& key)
		requires (requires (const KeyType& containerKey) { m_comparator(key, containerKey); })
	{
		return Eco_AVL_ELEM(FindInternal(key).hook);
	}

	/// @brief Find element by heterogeneous key.
	/// @param key Lookup key.
	/// @return Pointer to element or null.
	template<typename TKey>
	[[nodiscard]] const T* FindEquivalent(const TKey& key) const
		requires (requires (const KeyType& containerKey) { m_comparator(key, containerKey); })
	{
		return Eco_AVL_ELEM(FindInternal(key).hook);
	}


	/// @brief Insert new element into the tree.
	/// @param element Element to be inserted.
	/// @pre @p element is not part of any container.
	InsertResult Insert(T* const element)
	{
		auto const r = FindInternal(m_keySelector(*element));
		if (r.hook != nullptr) return { Eco_AVL_ELEM(r.hook), false };
		Core::Insert(Eco_AVL_HOOK(element), ConstCast<Ptr<Ptr<Hook>>>(r.parent));
		return { element, true };
	}

	/// @brief Remove an element from the tree.
	/// @param element Element to be removed.
	/// @pre @p element is part of this tree.
	void Remove(T* const element)
	{
		Core::Remove(Eco_AVL_HOOK(element));
	}

	/// @brief Remove all elements from the tree.
	using Core::Clear;

	/// @brief Flatten the tree into a linked list using an in-order traversal.
	[[nodiscard]] List<T> Flatten()
	{
		size_t const size = Size();
		return List<T>(static_cast<LinkContainer&&>(*this), Core::Flatten(), size);
	}


	/// @brief Create an iterator referring to an element.
	/// @pram element Element to which the resulting iterator shall refer.
	/// @pre @p element is part of this tree.
	[[nodiscard]] iterator MakeIterator(T* const element)
	{
		LinkCheck(*element, *this);
		return iterator(Eco_AVL_HOOK(element));
	}

	/// @brief Create an iterator referring to an element.
	/// @pram element Element to which the resulting iterator shall refer.
	/// @pre @p element is part of this tree.
	[[nodiscard]] const_iterator MakeIterator(const T* const element) const
	{
		LinkCheck(*element, *this);
		return const_iterator(Eco_AVL_HOOK(const_cast<T*>(element)));
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


	[[nodiscard]] friend size_t size(const AvlSet& set)
	{
		return set.Size();
	}

	friend void swap(AvlSet& lhs, AvlSet& rhs) noexcept
	{
		using std::swap;
		swap(static_cast<Core&>(lhs), static_cast<Core&>(rhs));
		swap(lhs.m_keySelector, rhs.m_keySelector);
		swap(lhs.m_comparator, rhs.m_comparator);
	}


#if Eco_AVL_DEBUG
	int DebugPrint(Hook* hook, size_t indent, int num)
	{
		if (Hook* const child = hook->children[1].Ptr())
			num = DebugPrint(child, indent + 1, num);

		std::cout << std::format("{:016x} ", reinterpret_cast<uintptr_t>(hook));
		int balance = (int)hook->children[1].Tag() - (int)hook->children[0].Tag();
		char balanceString[3] = { ' ', (char)('0' + (balance & 1)), '\0' };
		if (balance < 0) balanceString[0] = '-';
		if (balance > 0) balanceString[0] = '+';

		for (size_t i = 0; i < indent; ++i) std::cout << '\t';
		std::cout << std::format("{} {{{}}}\n", balanceString, num < 0 ? Eco_AVL_ELEM(hook)->value : num++);

		if (Hook* const child = hook->children[0].Ptr())
			num = DebugPrint(child, indent + 1, num);

		return num;
	}

	void DebugPrint(bool nums = false)
	{
		if (!m_debugPrintEnable) return;

		if (m_root.Value != nullptr)
		{
			DebugPrint(m_root->Ptr(), 0, -!nums);
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
		const Ptr<Hook>* parent = &m_root.Value;
		uintptr_t l = 0;

		while (!parent[l].IsZero())
		{
			Hook* const child = parent[l].Ptr();

			auto const ordering = m_comparator(key, m_keySelector(*Eco_AVL_ELEM(child)));
			if (ordering == 0) return { child, { parent, l } };

			parent = child->children;
			l = ordering > 0;
		}

		return FindResult{ nullptr, { parent, l } };
	}
};

#undef Eco_AVL_HOOK
#undef Eco_AVL_ELEM

} // namespace Private::AvlSet_

using Private::AvlSet_::AvlSet;

// } // inline namespace Eco_NS
} // namespace Eco
