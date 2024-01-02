#pragma once
#include <stdio.h>
#include <string>

namespace utl {

template <typename... ARGS>
std::string strPrintf(char const *fmt, ARGS &&... args)
{
    auto strToC = [](auto &a) {
        if constexpr (std::is_same_v<std::remove_cvref_t<decltype(a)>, std::string>)
            return a.c_str();
        else
            return a;
    };
    int size = snprintf(nullptr, 0, fmt, strToC(args)...);
    if (size <= 0)
        return std::string();
    std::string buf(size + 1, ' ');
    snprintf(const_cast<char *>(buf.c_str()), buf.size(), fmt, std::forward<ARGS>(args)...);
    return buf;
}

} // utl