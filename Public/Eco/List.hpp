#pragma once

#include "Eco/Assert.hpp"
#include "Eco/Link.hpp"

#include <concepts>
#include <type_traits>

namespace Eco {
// inline namespace Eco_NS {

using ListLink = Link<2>;

namespace Private::List_ {

#define Eco_LIST_HOOK(element) \
	(reinterpret_cast<Hook*>(static_cast<ListLink*>(element)))

#define Eco_LIST_ELEM(hook) \
	(static_cast<T*>(reinterpret_cast<ListLink*>(hook)))

struct Hook : LinkBase
{
	Hook* siblings[2];
};

struct Core : LinkContainer
{
	Hook m_root;
	size_t m_size = 0;

	constexpr Core()
	{
		m_root.siblings[0] = &m_root;
		m_root.siblings[1] = &m_root;
	}

	Core(Core&& src);

	Core& operator=(Core&&) = delete;

	void Adopt(Hook* head, size_t size);
	void Insert(Hook* prev, Hook* hook, bool before);
	void Remove(Hook* hook);
};


class IteratorCore
{
protected:
	Hook* m_hook;

public:
	IteratorCore() = default;

	IteratorCore(Hook* const hook)
		: m_hook(hook)
	{
	}

	bool operator==(const IteratorCore&) const = default;
};

template<typename T, bool const TDirection = 0>
struct Iterator : IteratorCore
{
	using difference_type = ptrdiff_t;
	using value_type = T;
	using pointer = T*;
	using reference = T&;


	using IteratorCore::IteratorCore;

#if 0
	Iterator(Iterator<std::remove_const_t<T>, TDirection> const iterator)
		requires std::is_const_v<T>
		: IteratorCore(static_cast<IteratorCore>(iterator))
	{
	}
#endif


	[[nodiscard]] T& operator*() const
	{
		return *Eco_LIST_ELEM(m_hook);
	}

	[[nodiscard]] T* operator->() const
	{
		return Eco_LIST_ELEM(m_hook);
	}


	Iterator& operator++()
	{
		m_hook = m_hook->siblings[TDirection];
		return *this;
	}

	[[nodiscard]] Iterator operator++(int)
	{
		auto it = *this;
		m_hook = m_hook->siblings[TDirection];
		return it;
	}

	Iterator& operator--()
	{
		m_hook = m_hook->siblings[!TDirection];
		return *this;
	}

	[[nodiscard]] Iterator operator--(int)
	{
		auto it = *this;
		m_hook = m_hook->siblings[!TDirection];
		return it;
	}


	[[nodiscard]] bool operator==(const Iterator&) const = default;
};


template<std::derived_from<ListLink> T>
class List : Core
{
public:
	using       iterator = Iterator<      T>;
	using const_iterator = Iterator<const T>;


	List() = default;

	// Adopting constructor for internal use only.
	List(LinkContainer&& container, Hook* const list, size_t const size);


	/// @return Size of the list.
	[[nodiscard]] size_t Size() const
	{
		return m_size;
	}

	/// @return True if the list is empty.
	[[nodiscard]] bool IsEmpty()
	{
		return m_size == 0;
	}


	/// @return First element in the list.
	/// @pre The list is not empty.
	[[nodiscard]] T* First()
	{
		Eco_Assert(m_size > 0);
		return Eco_LIST_ELEM(m_root.siblings[0]);
	}

	/// @return First element in the list.
	/// @pre The list is not empty.
	[[nodiscard]] const T* First() const
	{
		Eco_Assert(m_size > 0);
		return Eco_LIST_ELEM(m_root.siblings[0]);
	}

	/// @return Last element in the list.
	/// @pre The list is not empty.
	[[nodiscard]] T* Last()
	{
		Eco_Assert(m_size > 0);
		return Eco_LIST_ELEM(m_root.siblings[1]);
	}

	/// @return Last element in the list.
	/// @pre The list is not empty.
	[[nodiscard]] const T* Last() const
	{
		Eco_Assert(m_size > 0);
		return Eco_LIST_ELEM(m_root.siblings[1]);
	}


	/// @brief Insert an element at the front of the list.
	/// @param element Element to be inserted.
	/// @pre @p element is not part of any container.
	void Prepend(T* const element)
	{
		Core::Insert(&m_root, Eco_LIST_HOOK(element), 0);
	}

	void PrependList(List<T>& list);

	/// @brief Insert an element at the back of the list.
	/// @param element Element to be inserted.
	/// @pre @p element is not part of any container.
	void Append(T* const element)
	{
		Core::Insert(&m_root, Eco_LIST_HOOK(element), 1);
	}

	void AppendList(List<T>& list);

	/// @brief Insert an element before another element.
	/// @param existing Existing element positioned after the new element.
	/// @param element Element to be inserted.
	/// @pre @p existing is part of this container.
	/// @pre @p element is not part of any container.
	void InsertBefore(T* const existing, T* const element)
	{
		Core::Insert(Eco_LIST_HOOK(existing), Eco_LIST_HOOK(element), 0);
	}

	void InsertListBefore(T* const existing, List<T>& list);

	/// @brief Insert an element after another element.
	/// @param existing Existing element positioned before the new element.
	/// @param element Element to be inserted.
	/// @pre @p existing is part of this container.
	/// @pre @p element is not part of any container.
	void InsertAfter(T* const existing, T* const element)
	{
		Core::Insert(Eco_LIST_HOOK(existing), Eco_LIST_HOOK(element), 1);
	}

	void InsertListAfter(T* const existing, List<T>& list);

	/// @brief Remove an element from the list.
	/// @param element Element to be removed.
	/// @pre @p element is part of this list.
	void Remove(T* const element)
	{
		Core::Remove(Eco_LIST_HOOK(element));
	}

	List<T> RemoveList(T* const begin, T* const end);


	/// @brief Create an iterator referring to an element.
	/// @pram element Element to which the resulting iterator shall refer.
	/// @pre @p element is part of this list.
	[[nodiscard]] iterator MakeIterator(T* const element)
	{
		LinkCheck(*element, *this);
		return iterator(Eco_LIST_HOOK(element));
	}

	/// @brief Create an iterator referring to an element.
	/// @pram element Element to which the resulting iterator shall refer.
	/// @pre @p element is part of this list.
	[[nodiscard]] const_iterator MakeIterator(const T* const element) const
	{
		LinkCheck(*element, *this);
		return const_iterator(Eco_LIST_HOOK(const_cast<T*>(element)));
	}


	[[nodiscard]] iterator begin()
	{
		return iterator(m_root.siblings[0]);
	}

	[[nodiscard]] const_iterator begin() const
	{
		return const_iterator(m_root.siblings[0]);
	}

	[[nodiscard]] iterator end()
	{
		return iterator(&m_root);
	}

	[[nodiscard]] const_iterator end() const
	{
		return const_iterator(const_cast<Hook*>(&m_root));
	}



	[[nodiscard]] friend size_t size(const List& list)
	{
		return list.Size();
	}

	friend void swap(List& lhs, List& rhs)
	{
		using std::swap;
		swap(static_cast<Core&>(lhs), static_cast<Core&>(rhs));
	}
};

#undef Eco_LIST_HOOK
#undef Eco_LIST_ELEM

} // namespace Private::List_

using Private::List_::List;

// } // inline namespace Eco_NS
} // namespace Eco
