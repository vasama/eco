#pragma once

#include "Eco/Concepts.hpp"

namespace Eco {
// inline namespace Eco_NS {

struct IdentityKeySelector
{
	template<typename T>
	const T& operator()(const T& k) const
	{
		return k;
	}
};

struct PointerKeySelector
{
	template<typename T>
	const T* operator()(const T& k) const
	{
		return &k;
	}
};

template<typename TSelector, typename T>
concept KeySelector = requires (const TSelector& selector, const T& object)
{
	{ selector(object) } -> not_same_as<void>;
};

// } // inline namespace Eco_NS
} // namespace Eco
