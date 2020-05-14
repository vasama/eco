#include "Eco/Assert.hpp"

#include <cstdio>
#include <cstdlib>

#include <Windows.h>

bool Eco::AssertCallback(const char* const expr, const char* const file, int const line, const char* const fmt, ...)
{
	char buffer[2048];
	size_t const size = snprintf(
		buffer, sizeof(buffer),
		"Assertion failure.\r\n\r\n"
		"File: %s\r\n"
		"Line: %d\r\n\r\n"
		"Expression: (%s)\r\n\r\n",
		file, line, expr);

	if (fmt != nullptr && size < sizeof(buffer) - 1)
	{
		va_list vlist;
		va_start(vlist, fmt);
		vsnprintf(buffer + size, sizeof(buffer) - size, fmt, vlist);
		va_end(vlist);
	}

	switch (MessageBoxA(NULL, buffer, "Eco Assert Handler",
		MB_ABORTRETRYIGNORE | MB_ICONERROR | MB_SYSTEMMODAL | MB_SETFOREGROUND))
	{
	case IDRETRY: return true;
	case IDIGNORE: return false;
	}

	std::abort();
}
