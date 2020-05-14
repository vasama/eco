#pragma once

#ifndef Eco_CONFIG_DEBUG
#	define Eco_CONFIG_DEBUG 1
#endif

#ifdef __clang__
#	define Eco_CLANG_DIAG_1(...) #__VA_ARGS__
#	define Eco_CLANG_DIAG(...) _Pragma(Eco_CLANG_DIAG_1(clang diagnostic __VA_ARGS__))
#else
#	define Eco_CLANG_DIAG(...)
#endif
