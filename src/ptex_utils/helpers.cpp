#include <cstring>

#include "helpers.hpp"

std::string strbasename(const char* s)
{
    if (!s || !*s)
        return ".";
    size_t i = std::strlen(s)-1;
    size_t end = 0;
    for (; i && s[i] == '/'; i--){}
    end = i+1;
    for (; i && s[i-1] != '/'; i--){}

    return std::string(s+i, end);
}
