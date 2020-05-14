#pragma once

#include "Eco/Private/Config.hpp"

namespace Eco {
// inline namespace Eco_NS {

inline constexpr struct{} AdoptLock = {};

template<typename T>
class Lock
{
	T& m_resource;

public:
	Lock(T& resource)
		: m_resource(resource)
	{
		m_resource.Lock();
	}

	Lock(T& resource, decltype(AdoptLock))
		: m_resource(resource)
	{
	}

	Lock(const Lock&) = delete;
	Lock& operator=(const Lock&) = delete;

	~Lock()
	{
		m_resource.Unlock();
	}
};

// } // inline namespace Eco_NS
} // namespace Eco
