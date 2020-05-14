#pragma once

#ifndef Eco_CONFIG_LINK_DEBUG
#	define Eco_CONFIG_LINK_DEBUG 0
#endif

#include "Eco/Assert.hpp"

#if Eco_CONFIG_LINK_DEBUG
#	include <utility>
#endif

#include <cstdint>

namespace Eco {

class LinkBase;

#if Eco_CONFIG_LINK_DEBUG
struct LinkShared;

class LinkSharedHandle
{
protected:
	LinkShared* m_shared = nullptr;

	LinkSharedHandle() = default;

	LinkSharedHandle(LinkSharedHandle&& src) noexcept
		: m_shared(std::exchange(src.m_shared, nullptr))
	{
	}

	LinkSharedHandle& operator=(LinkSharedHandle&& src) noexcept
	{
		if (LinkShared* const shared = std::exchange(m_shared, std::exchange(src.m_shared, nullptr)))
			ReleaseShared(shared);
		return *this;
	}

	~LinkSharedHandle()
	{
		if (m_shared != nullptr)
			ReleaseShared(m_shared);
	}

	static void ReleaseShared(LinkShared* shared);

	friend void swap(LinkSharedHandle& lhs, LinkSharedHandle& rhs) noexcept
	{
		std::swap(lhs.m_shared, rhs.m_shared);
	}
};
#endif

class LinkContainer
#if Eco_CONFIG_LINK_DEBUG
	: LinkSharedHandle
#endif
{
protected:
	LinkContainer() = default;

private:
#if Eco_CONFIG_LINK_DEBUG
	friend void LinkInsertInternal(LinkContainer& container, LinkBase& link);
	friend void LinkRemoveInternal(LinkContainer& container, LinkBase& link);
#endif

	friend class LinkBase;
};

class LinkBase
#if Eco_CONFIG_LINK_DEBUG
	: LinkSharedHandle
#endif
{
protected:
	LinkBase() = default;

#if Eco_CONFIG_LINK_DEBUG
	LinkBase(LinkBase&& src) = default;

	LinkBase(const LinkBase& src)
	{
	}

	LinkBase& operator=(LinkBase&& src) = default;

	LinkBase& operator=(const LinkBase& src)
	{
		return *this;
	}

	~LinkBase()
	{
		Eco_Assert(m_shared == nullptr);
	}
#endif

private:
#if Eco_CONFIG_LINK_DEBUG
	friend void LinkInsertInternal(LinkContainer& container, LinkBase& link);
	friend void LinkRemoveInternal(LinkContainer& container, LinkBase& link);
#endif

	friend void LinkInsert(LinkBase& link, LinkContainer& container)
	{
#if Eco_CONFIG_LINK_DEBUG
		Eco_Assert(link.m_shared == nullptr);
		LinkInsertInternal(container, link);
#endif
	}

	friend void LinkRemove(LinkBase& link, LinkContainer& container)
	{
#if Eco_CONFIG_LINK_DEBUG
		Eco_Assert(link.m_shared != nullptr);
		Eco_Assert(link.m_shared == container.m_shared);
		LinkRemoveInternal(container, link);
#endif
	}

	friend void LinkCheck(const LinkBase& link, const LinkContainer& container)
	{
#if Eco_CONFIG_LINK_DEBUG
		Eco_Assert(link.m_shared != nullptr);
		Eco_Assert(link.m_shared == container.m_shared);
#endif
	}

	friend class LinkContainer;
};


template<size_t TSize>
	requires (TSize > 0)
class Link : public Link<TSize - 1>
{
	[[maybe_unused]]
	uintptr_t data;
};

template<>
class Link<1> : public LinkBase
{
	[[maybe_unused]]
	uintptr_t data;
};

} // namespace Eco
