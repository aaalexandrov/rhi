#pragma once

#include <iostream>

namespace utl {

#define JOIN1(A, B) A ## B
#define JOIN(A, B) JOIN1(A, B)

#define STRINGIFY1(A) #A
#define STRINGIFY(A) STRINGIFY1(A)

#if defined(NDEBUG)
#	define ASSERT(expression) (void)sizeof(expression)
#else
#	define ASSERT(expression) (void) ((!!(expression)) || (utl::AssertFailed(#expression, __FILE__, (unsigned)__LINE__), 0))
#endif

void AssertFailed(char const *message, char const *file, unsigned line);

#if defined(NDEBUG)
#	define LOG(...) ((void)(__VA_ARGS__))
#else
#	define LOG(...) utl::LogLine(std::cerr, __VA_ARGS__)
#endif


template <typename ARG, typename... ARGS>
void LogLine(std::ostream &out, ARG &&arg, ARGS&&... args)
{
	out << std::forward<ARG>(arg);
	using expander = int[];
	static_cast<void>(expander{ 0, (void(out << std::forward<ARGS>(args)), 0)... });
	out << std::endl;
}


}