#pragma once

#include <type_traits>

#include <assert.h>
#include <stdint.h>

namespace Eco {

template<typename T>
	requires(alignof(T) > 1)
class TaggedPointer
{
	static constexpr uintptr_t TagMask = (uintptr_t)alignof(T) - 1;
	static constexpr uintptr_t PtrMask = ~TagMask;

	uintptr_t m_ptr;

public:
	TaggedPointer()
	{
		m_ptr = nullptr;
	}

	TaggedPointer(decltype(nullptr))
	{
		m_ptr = nullptr;
	}

	explicit TaggedPointer(T* ptr)
	{
		m_ptr = (uintptr_t)ptr;
	}

	TaggedPointer(T* ptr, uintptr_t tag)
	{
		assert(tag <= TagMask);
		m_ptr = (uintptr_t)ptr | tag;
	}

	TaggedPointer(const TaggedPointer&) = default;
	TaggedPointer& operator=(const TaggedPointer&) = default;

	T* Ptr() const
	{
		return (T*)(m_ptr & PtrMask);
	}

	void SetPtr(T* ptr)
	{
		m_ptr = (uintptr_t)ptr | (m_ptr & TagMask);
	}

	uintptr_t Tag() const
	{
		return m_ptr & TagMask;
	}

	void SetTag(uintptr_t tag)
	{
		assert(tag <= TagMask);
		m_ptr = m_ptr | (m_ptr & PtrMask);
	}
};

} // namespace Eco

