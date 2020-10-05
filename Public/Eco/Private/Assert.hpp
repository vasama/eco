#pragma once

#include "Eco/Private/Config.hpp"
#include "Eco/Private/PP.h"

#ifndef Eco_ASSERT_LEVEL
#	define Eco_ASSERT_LEVEL Eco_CONFIG_ASSERT
#endif

namespace Eco {
inline namespace Eco_NS {

bool AssertCallback(const char* expr, const char* file, int line, const char* fmt = nullptr, ...);

#define Eco_Assert_1(cond, ...) ((cond) ? (void)0 : (::Eco::Eco_NS::AssertCallback(Eco_PP_STR(cond), __FILE__, __LINE__, ##__VA_ARGS__) ? Eco_DEBUGBREAK() : (void)0))

} // inline namespace Eco_NS
} // namespace Eco
