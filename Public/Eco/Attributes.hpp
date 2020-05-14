#pragma once

#ifdef _MSC_VER
#	define Eco_NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#else
#	define Eco_NO_UNIQUE_ADDRESS [[no_unique_address]]
#endif
