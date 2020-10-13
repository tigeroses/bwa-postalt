
#include "postalt/altfile.h"
#include "postalt/utility.h"

#include <iostream>
#include <fstream>
#include <regex>
#include <exception>
#include <stdexcept>

using namespace postalt;

Altfile::Altfile(std::string file) :
    m_filename(file)
{

}

void Altfile::parse()
{
    std::ifstream ifs(m_filename);
    std::string line;
    while (std::getline(ifs, line))
    {
        // Skip the header section
        if (line.at(0) == '@') continue;
        std::vector<std::string> vec_s = split_str(line, '\t');
        // Incomplete lines
        if (vec_s.size() < 11) continue;
        m_is_alt.insert(vec_s[0]);
        int pos = std::stoi(vec_s[3]) - 1;
        int flag = std::stoi(vec_s[1]);
        // Unmap or no chromosome
        if ((flag & 4) || vec_s[2] == "*") continue;

        // Read HLA contigs
        std::regex hla_regex(R"(^(HLA-[^\s\*]+)\*\d+)");
        std::smatch hla_match;
        if (std::regex_search(vec_s[0], hla_match, hla_regex))
        {
            // std::cout<<hla_match[1]<<std::endl;
            ++m_hla_ctg[hla_match[1]];
            m_hla_chr = vec_s[2];
        }

        // Process cigar info
        int l_qaln = 0, l_tlen = 0, l_qclip = 0;
        std::vector<std::pair<char, int>> cigar;
        std::regex cigar_regex(R"((\d+)([MIDSHN]))");
        std::smatch cigar_match;
        auto cigar_seq = vec_s[5];
        while (std::regex_search(cigar_seq, cigar_match, cigar_regex))
        {
            // std::cout<<cigar_match[1]<<" "<<cigar_match[2]<<std::endl;
            int l = std::stoi(cigar_match[1]);
            auto c = cigar_match.str(2).at(0);
            cigar.push_back({c != 'H' ? c : 'S', l});

            if (c == 'M') l_qaln += l, l_tlen += l;
            else if (c == 'I') l_qaln += l;
            else if (c == 'S' || c == 'H') l_qclip += l;
            else if (c == 'D' || c == 'N') l_tlen += l;
            
            // Get the unmatch string
            cigar_seq = cigar_match.suffix();
        }

        // Flag is reverse
        int j = flag & 16 ? cigar.size() - 1 : 0;
        int start = cigar[j].first == 'S' ? cigar[j].second : 0;
        IntvAlt intv_alt{start, start + l_qaln, l_qaln + l_qclip, vec_s[2],
            flag & 16 ? true : false, pos - 1, cigar, pos + l_tlen};
        m_intv_alt[vec_s[0]].push_back(std::move(intv_alt));
        m_intv_pri[vec_s[2]].push_back({pos, pos + l_tlen, vec_s[0]});
    }
    ifs.close();

}