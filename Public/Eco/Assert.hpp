#include "Eco/Private/Assert.hpp"

#ifdef Eco_Assert
#	undef Eco_Assert
#	undef Eco_Verify
#endif

#if Eco_ASSERT_LEVEL
	#define Eco_Assert(cond, ...) Eco_Assert_1(cond, ##__VA_ARGS__)
	#define Eco_Verify(cond, ...) Eco_Assert_1(cond, ##__VA_ARGS__)
#else
	#define Eco_Assert(cond, ...) ((void)0)
	#define Eco_Verify(cond, ...) ((void)(cond))
#endif
