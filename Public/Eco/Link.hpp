#pragma once

#include "Eco/Private/Config.hpp"

namespace Eco {
inline namespace Eco_NS {

template<typename T>
struct SingleLink
{
	SingleLink<T>* m_next;
};

template<typename T>
struct DoubleLink : SingleLink<T>
{
	DoubleLink<T>* m_prev;
};

} // inline namespace Eco_NS
} // namespace Eco
