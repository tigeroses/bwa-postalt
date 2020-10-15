
#pragma once

#include <string>
#include <vector>

std::vector< std::string > split_str(const std::string& str, char delim = ' ', bool skip_empty = true);

std::string cat_str(std::vector<std::string>& l, char sep = ' ');