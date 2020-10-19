
#pragma once

#include <string>
#include <vector>

void split_str(const std::string& str, std::vector<std::string>& v, char delim = ' ', bool skip_empty = true);

std::string cat_str(std::vector<std::string>& l, char sep = ' ', bool endline = false);

std::vector<std::pair<int, char>> parse_cigar(std::string& s);
