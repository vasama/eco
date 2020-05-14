#pragma once

#include "Eco/Private/PP.h"

#ifndef Eco_CONFIG_ASSERT
#	define Eco_CONFIG_ASSERT 2
#endif

#if Eco_CONFIG_ASSERT
#	define Eco_NS_ASSERT _a
#else
#	define Eco_NS_ASSERT
#endif

#define Eco_NS_2(a) v1 ## a
#define Eco_NS_1(...) Eco_NS_2(__VA_ARGS__)
#define Eco_NS Eco_NS_1(Eco_NS_ASSERT)

#if defined(__clang__) || defined(__GNUC__)
#	define Eco_FORCEINLINE __attribute__((always_inline))
#	define Eco_FORCENOINLINE __attribute__((noinline))
#elif defined(_MSC_VER)
#	define Eco_FORCEINLINE __forceinline
#	define Eco_FORCENOINLINE __declspec(noinline)
#else
#	error unsupported compiler
#endif

#ifdef __clang__
#	define Eco_CLANG_DIAG(...) _Pragma(Eco_PP_STR(clang diagnostic __VA_ARGS__))
#else
#	define Eco_CLANG_DIAG(...)
#endif

#ifdef _WIN32
#	define Eco_DEBUGBREAK() __debugbreak()
#else
#	define Eco_DEBUGBREAK() ((void)0)
#endif
