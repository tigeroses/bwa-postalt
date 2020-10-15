
#include "postalt/utility.h"

#include <sstream>

std::vector< std::string > split_str(const std::string& str, char delim, bool skip_empty)
{
    std::istringstream iss(str);
    std::vector< std::string >   res;
    for (std::string item; std::getline(iss, item, delim);)
        if (skip_empty && item.empty())
            continue;
        else
            res.push_back(item);
    return res;
}

std::string cat_str(std::vector<std::string>& l, char sep, bool endline)
{
    std::stringstream ss;
    for (size_t i = 0; i < l.size(); ++i)
    {
        ss << l[i];
        if (i != (l.size()-1))
            ss << sep;
    }
    if (endline)
        ss << '\n';
    return ss.str();
}

