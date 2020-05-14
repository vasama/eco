#pragma once

#include "Eco/Assert.hpp"
#include "Eco/Private/Config.hpp"

#include <algorithm>
#include <limits>
#include <type_traits>

#include <cstdint>

namespace Eco {
// inline namespace Eco_NS {

namespace Private::TaggedPointer_ {

template<typename T, typename TTag, TTag TMax>
class IncompleteTaggedPointer;

template<typename TIn, typename TOut, typename TInTag, typename TOutTag, TInTag TInMax, TOutTag TOutMax>
void StaticCastConstraint(const IncompleteTaggedPointer<TIn, TInTag, TInMax>&, const IncompleteTaggedPointer<TOut, TOutTag, TOutMax>&)
	requires requires (TIn* in, TInTag inTag) { static_cast<TOut*>(in); static_cast<TOutTag>(inTag); } && (static_cast<TOutTag>(TInMax) <= TOutMax);

template<typename TOut, typename TIn, typename TInTag, TInTag TInMax>
TOut StaticCast(const IncompleteTaggedPointer<TIn, TInTag, TInMax>& ptr)
	requires requires (const TOut& out) { StaticCastConstraint(ptr, out); }
{
	return TOut(ptr.m_value);
}

template<typename TIn, typename TOut, typename TTag, TTag TMax>
void ConstCastConstraint(const IncompleteTaggedPointer<TIn, TTag, TMax>&, const IncompleteTaggedPointer<TOut, TTag, TMax>&)
	requires requires (TIn* in) { const_cast<TOut*>(in); };

template<typename TOut, typename TIn, typename TInTag, TInTag TInMax>
TOut ConstCast(const IncompleteTaggedPointer<TIn, TInTag, TInMax>& ptr)
	requires requires (const TOut& out) { ConstCastConstraint(ptr, out); }
{
	return TOut(ptr.m_value);
}

template<typename TIn, typename TOut, typename TTag, TTag TMax>
void ReinterpretCastConstraint(const IncompleteTaggedPointer<TIn, TTag, TMax>&, const IncompleteTaggedPointer<TOut, TTag, TMax>&)
	requires requires (TIn* in) { reinterpret_cast<TOut*>(in); };

template<typename TOut, typename TIn, typename TInTag, TInTag TInMax>
TOut ReinterpretCast(const IncompleteTaggedPointer<TIn, TInTag, TInMax>& ptr)
	requires requires (const TOut& out) { ReinterpretCastConstraint(ptr, out); }
{
	return TOut(ptr.m_value);
}

template<typename T, typename TTag, TTag TMax>
class IncompleteTaggedPointer
{
	// Laundering the value through integral_constant allows the Visual Studio debugger to see the value.
	static constexpr uintptr_t TagMask = std::integral_constant<uintptr_t,
		(static_cast<uintptr_t>(-1) >> std::countl_zero(static_cast<uintptr_t>(TMax)))>::value;

	static constexpr uintptr_t PtrMask = ~TagMask;

public:
	using ValueType = T;
	using TagType = TTag;

	static constexpr TTag MaxTag = TMax;

	class TagProxyType
	{
		uintptr_t& m_value;

	public:
		TagProxyType(uintptr_t& value)
			: m_value(value)
		{
		}

		TagProxyType& operator=(TTag const tag)
		{
			Eco_Assert(tag <= MaxTag);
			m_value = tag;
			return *this;
		}

		TagProxyType& operator&=(TTag const tag)
		{
			Eco_Assert(tag <= MaxTag);
			m_value &= tag;
			return *this;
		}

		TagProxyType& operator|=(TTag const tag)
		{
			Eco_Assert(tag <= MaxTag);
			m_value |= tag;
			return *this;
		}

		TagProxyType& operator^=(TTag const tag)
		{
			Eco_Assert(tag <= MaxTag);
			m_value ^= tag;
			return *this;
		}

		bool operator==(TTag const tag) const
		{
			return static_cast<TTag>(m_value & TagMask) == tag;
		}

		bool operator!=(TTag const tag) const
		{
			return static_cast<TTag>(m_value & TagMask) != tag;
		}

		operator TTag() const
		{
			return static_cast<TTag>(m_value & TagMask);
		}
	};

	class PtrProxyType
	{
		uintptr_t& m_value;

	public:
		PtrProxyType(uintptr_t& value)
			: m_value(value)
		{
		}

		PtrProxyType& operator=(T* const ptr)
		{
			m_value = reinterpret_cast<uintptr_t>(ptr) | (m_value & TagMask);
			return *this;
		}

		bool operator==(T* const ptr) const
		{
			return reinterpret_cast<T*>(m_value & PtrMask) == ptr;
		}

		bool operator!=(T* const ptr) const
		{
			return reinterpret_cast<T*>(m_value & PtrMask) != ptr;
		}

		operator T*() const
		{
			return reinterpret_cast<T*>(m_value & PtrMask);
		}
	};

private:
	uintptr_t m_value;

public:
	IncompleteTaggedPointer() = default;

	constexpr IncompleteTaggedPointer(decltype(nullptr))
		: m_value(0)
	{
	}

	IncompleteTaggedPointer(T* const ptr)
		: m_value(reinterpret_cast<uintptr_t>(ptr))
	{
	}

	IncompleteTaggedPointer(T* const ptr, TTag const tag)
		: m_value(reinterpret_cast<uintptr_t>(ptr) | static_cast<uintptr_t>(tag))
	{
		Eco_Assert(static_cast<uintptr_t>(tag) <= TagMask);
	}

	template<typename TOther, typename TOtherTag, TOtherTag TOtherMax>
	IncompleteTaggedPointer(const IncompleteTaggedPointer<TOther, TOtherTag, TOtherMax>& other)
		requires std::is_convertible_v<TOther*, T*> && std::is_convertible_v<TOtherTag, TTag> && (static_cast<TTag>(TOtherMax) <= TMax)
		: m_value(other.m_value)
	{
	}

	IncompleteTaggedPointer(const IncompleteTaggedPointer&) = default;
	IncompleteTaggedPointer& operator=(const IncompleteTaggedPointer&) = default;


	T* Ptr() const
	{
		return reinterpret_cast<T*>(m_value & PtrMask);
	}

	void SetPtr(T* const ptr)
	{
		m_value = reinterpret_cast<uintptr_t>(ptr) | (m_value & TagMask);
	}

	TTag Tag() const
	{
		return static_cast<TTag>(m_value & TagMask);
	}

	void SetTag(TTag const tag)
	{
		Eco_Assert(static_cast<uintptr_t>(tag) <= TagMask);
		m_value = (m_value & PtrMask) | static_cast<uintptr_t>(tag);
	}

	TagProxyType TagProxy()
	{
		return TagProxyType(m_value);
	}

	void Set(T* const ptr, TTag const tag)
	{
		Eco_Assert(static_cast<uintptr_t>(tag) <= TagMask);
		m_value = reinterpret_cast<uintptr_t>(ptr) | static_cast<uintptr_t>(tag);
	}

	bool IsZero() const
	{
		return m_value == 0;
	}


	T& operator*() const
	{
		return *reinterpret_cast<T*>(m_value & PtrMask);
	}

	T* operator->() const
	{
		return reinterpret_cast<T*>(m_value & PtrMask);
	}

	T& operator[](ptrdiff_t index) const
	{
		return reinterpret_cast<T*>(m_value & PtrMask)[index];
	}


	bool operator==(const IncompleteTaggedPointer&) const = default;

	bool operator==(decltype(nullptr)) const
	{
		return (m_value & PtrMask) == 0;
	}

	bool operator!=(decltype(nullptr)) const
	{
		return (m_value & PtrMask) != 0;
	}


	static constexpr bool CheckCompleteType()
	{
		return static_cast<TTag>(alignof(T)) > TMax;
	}

private:
	IncompleteTaggedPointer(uintptr_t const value)
		: m_value(value)
	{
	}

	template<typename TOther, typename TOtherTag, TOtherTag TOtherMax>
	friend class IncompleteTaggedPointer;

	template<typename TOut, typename TIn, typename TInTag, TInTag TInMax>
	friend TOut StaticCast(const IncompleteTaggedPointer<TIn, TInTag, TInMax>& ptr)
		requires requires (const TOut& out) { StaticCastConstraint(ptr, out); };

	template<typename TOut, typename TIn, typename TInTag, TInTag TInMax>
	friend TOut ConstCast(const IncompleteTaggedPointer<TIn, TInTag, TInMax>& ptr)
		requires requires (const TOut& out) { ConstCastConstraint(ptr, out); };

	template<typename TOut, typename TIn, typename TInTag, TInTag TInMax>
	friend TOut ReinterpretCast(const IncompleteTaggedPointer<TIn, TInTag, TInMax>& ptr)
		requires requires (const TOut& out) { ReinterpretCastConstraint(ptr, out); };
};


template<typename T, typename TTag>
consteval TTag MaxTag()
{
	if constexpr (std::is_enum_v<TTag>)
		return static_cast<TTag>(MaxTag<T, std::underlying_type_t<TTag>>());

	if constexpr (std::is_signed_v<TTag>)
		return static_cast<TTag>(MaxTag<T, std::make_unsigned_t<TTag>>() / 2);

	uintptr_t max = alignof(T) - 1;

	if (max > std::numeric_limits<TTag>::max())
		max = std::numeric_limits<TTag>::max();

	return max;
}

template<typename T, typename TTag>
consteval bool CheckMaxTag(TTag const max)
{
	if constexpr (std::is_enum_v<TTag>)
		return CheckMaxTag<T>(static_cast<std::underlying_type_t<TTag>>(max));

	return max > 0 && max <= MaxTag<T, TTag>();
}

template<typename T, typename TTag = uintptr_t, TTag TMax = MaxTag<T, TTag>()>
	requires (CheckMaxTag<T>(TMax))
using TaggedPointer = IncompleteTaggedPointer<T, TTag, TMax>;

} // namespace Private::TaggedPointer_

using Private::TaggedPointer_::IncompleteTaggedPointer;
using Private::TaggedPointer_::TaggedPointer;

template<typename T>
consteval bool CheckCompleteTaggedPointer()
{
	return Private::TaggedPointer_::CheckMaxTag<typename T::ValueType>(T::MaxTag);
}

// } // inline namespace Eco_NS
} // namespace Eco
