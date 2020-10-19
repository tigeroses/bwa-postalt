
#include "postalt/utility.h"

#include <sstream>

void split_str(const std::string& s, std::vector<std::string>& v, char c, bool skip_empty)
{
    // std::istringstream iss(str);
    // std::vector< std::string >   res;
    // for (std::string item; std::getline(iss, item, delim);)
    //     if (skip_empty && item.empty())
    //         continue;
    //     else
    //         res.push_back(item);
    // return res;

    v.clear();
    std::string::size_type pos1, pos2;
    pos2 = s.find(c);
    pos1 = 0;
    while(std::string::npos != pos2)
    {
        v.push_back(s.substr(pos1, pos2-pos1));

        pos1 = pos2 + 1;
        pos2 = s.find(c, pos1);
    }
    if(pos1 != s.length())
        v.push_back(s.substr(pos1));
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

std::vector<std::pair<int, char>> parse_cigar(std::string& s)
{
    std::vector<std::pair<int, char>> res;
    std::string len_str;
    for (auto& c : s)
    {
        if ('0' <= c && c <= '9')
            len_str += c;
        else
        {
            if (len_str.empty()) continue;
            res.push_back({std::stoi(len_str), c});
            len_str.clear();
        }
    }
    return res;
}


