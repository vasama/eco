#pragma once

#include "Eco/Private/Config.hpp"
#include "Eco/Private/PP.h"

#ifndef Eco_ASSERT_LEVEL
#	define Eco_ASSERT_LEVEL Eco_CONFIG_ASSERT
#endif

namespace Eco {
// inline namespace Eco_NS {

bool AssertCallback(const char* expr, const char* file, int line, const char* fmt = nullptr, ...);

#define Eco_Assert_2_(cond, ...) ( \
		::Eco::AssertCallback(Eco_PP_STR(cond), __FILE__, __LINE__, ##__VA_ARGS__) \
			? Eco_DEBUGBREAK() \
			: (void)0 \
	)

#define Eco_Assert_1_(cond, ...) ( \
		(cond) \
			? true \
			: (Eco_Assert_2_(cond, ##__VA_ARGS__), false) \
	)

// } // inline namespace Eco_NS
} // namespace Eco
