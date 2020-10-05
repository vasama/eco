#pragma once

#include "Eco/Assert.hpp"
#include "Eco/Private/Config.hpp"

#include <type_traits>

#include <cstdint>

namespace Eco {
inline namespace Eco_NS {

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
		Eco_Assert(tag <= TagMask);
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
		Eco_Assert(tag <= TagMask);
		m_ptr = m_ptr | (m_ptr & PtrMask);
	}
};

} // inline namespace Eco_NS
} // namespace Eco
