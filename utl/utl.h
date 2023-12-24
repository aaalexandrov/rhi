#pragma once
#include <stdio.h>
#include <string>

namespace utl {

template <typename... ARGS>
std::string strPrintf(char const *fmt, ARGS &&... args)
{
    int size = snprintf(nullptr, 0, fmt, args...);
    if (size <= 0)
        return std::string();
    std::string buf(size, ' ');
    snprintf(const_cast<char *>(buf.c_str()), size, fmt, std::forward<ARGS>(args)...);
    return buf;
}

} // utl