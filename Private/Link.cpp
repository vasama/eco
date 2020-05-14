#include "Eco/Link.hpp"

#if Eco_CONFIG_LINK_DEBUG

#include "Eco/Atomic.hpp"

using namespace Eco;

struct Eco::LinkShared
{
	atomic<intptr_t> m_refcount = 1;

	LinkShared() = default;

	LinkShared(const LinkShared&) = delete;
	LinkShared& operator=(const LinkShared&) = delete;
};

void LinkSharedHandle::ReleaseShared(LinkShared* const shared)
{
	Eco_Assert(shared != nullptr);
	if (shared->m_refcount.fetch_sub(1, std::memory_order::acq_rel) == 1)
		delete shared;
}

void Eco::LinkInsertInternal(LinkContainer& container, LinkBase& link)
{
	if (container.m_shared == nullptr)
		container.m_shared = new LinkShared();

	Eco_Verify(container.m_shared->m_refcount.fetch_add(1, std::memory_order::relaxed) > 0);
	link.m_shared = container.m_shared;
}

void Eco::LinkRemoveInternal(LinkContainer& container, LinkBase& link)
{
	Eco_Verify(container.m_shared->m_refcount.fetch_sub(1, std::memory_order::relaxed) > 1);
	link.m_shared = nullptr;
}

#endif
