#include "Eco/Private/Assert.hpp"

#ifdef Eco_Assert
#	undef Eco_Assert
#	undef Eco_Verify
#endif

#if Eco_ASSERT_LEVEL >= 1
	#define Eco_Assert(cond, ...) (void)Eco_Assert_1_(cond, ##__VA_ARGS__)
	#define Eco_Verify(cond, ...) Eco_Assert_1_(cond, ##__VA_ARGS__)
#else
	#define Eco_Assert(cond, ...) ((void)0)
	#define Eco_Verify(cond, ...) (static_cast<bool>(cond))
#endif

#if Eco_ASSERT_LEVEL >= 2
	#define Eco_AssertSlow(cond, ...) Eco_Assert(cond, ##__VA_ARGS__)
	#define Eco_VerifySlow(cond, ...) Eco_Verify(cond, ##__VA_ARGS__)
#else
	#define Eco_AssertSlow(cond, ...) ((void)0)
	#define Eco_VerifySlow(cond, ...) (static_cast<bool>(cond))
#endif
