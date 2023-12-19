#pragma once

#include <iostream>
#include <format>

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


template <typename... ARGS>
void LogLine(std::ostream &out, std::format_string<ARGS...> fmt, ARGS&&... args)
{
	out << std::format(fmt, std::forward<ARGS>(args)...) << std::endl;
}

}