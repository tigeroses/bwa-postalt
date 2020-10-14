
#include <postalt/postalt.h>
#include <postalt/utility.h>

#include <iostream>
#include <regex>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cmath>

using namespace postalt;

Postalt::Postalt(std::string alt_filename) : 
  m_alt_filename(alt_filename)
{
  // Read and parse alt file
  p_alt = new Altfile(m_alt_filename);
  p_alt->parse();
}

void Postalt::print_buffer(std::vector<std::vector<std::string>>& buff, std::string& out)
{
  if (buff.size() == 0) return;
  for (auto& b : buff)
    out += cat_str(b);
}

auto Postalt::parse_hit(std::string& contig, bool reverse, int start, std::string& cigar,
  int NM)
{
  Hit h;
  h.ctg = contig;
  h.start = start - 1;
  h.rev = reverse;
  h.cigar = cigar;
  h.hard = false;
  // Process cigar info
  int l_ins, n_ins, l_del, n_del, l_match, l_skip, l_clip;
	l_ins = l_del = n_ins = n_del = l_match = l_skip = l_clip = 0;
  std::regex cigar_regex(R"((\d+)([MIDSHN]))");
  std::smatch cigar_match;
  auto cigar_seq = cigar;
  while (std::regex_search(cigar_seq, cigar_match, cigar_regex))
  {
    // std::cout<<cigar_match[1]<<" "<<cigar_match[2]<<std::endl;
    int l = std::stoi(cigar_match[1]);
    auto c = cigar_match.str(2).at(0);
    if (c == 'M') l_match += l;
    else if (c == 'D') ++n_del, l_del += l;
		else if (c == 'I') ++n_ins, l_ins += l;
		else if (c == 'N') l_skip += l;
		else if (c == 'H' || c == 'S') 
    {
			l_clip += l;
			if (c == 'H') h.hard = true;
		}
    
    // Get the unmatch string
    cigar_seq = cigar_match.suffix();
  }
  h.end = h.start + l_match + l_del + l_skip;
  h.nm = std::max(NM, l_del + l_ins);
  h.score = floor((opt.a * l_match - (opt.a + opt.b) * (h.nm - l_del - l_ins) -
    opt.o * (n_del + n_ins) - opt.e * (l_del + l_ins)) / opt.a + 0.499);
  h.l_query = l_match + l_ins + l_clip;
  return h;
}

auto Postalt::parse_hit(std::string& xa_str)
{
  auto vec_s = split_str(xa_str, ',');
  bool reverse = vec_s[1].at(0) == '-';
  int start = std::stoi(vec_s[1].substr(1));
  return parse_hit(vec_s[0], reverse, start, vec_s[2], std::stoi(vec_s[3]));
}

int Postalt::cigar2pos(std::vector<std::pair<char, int>>& cigar, int pos)
{
  int x = 0, y = 0;
  for (auto& [op, len] : cigar)
  {
    if (op == 'M')
    {
      if (y <= pos && pos < y + len)
        return x + (pos - y);
      x += len;
      y += len;
    }
    else if (op == 'D')
      x += len;
    else if (op == 'I')
    {
      if (y <= pos && pos < y + len)
        return x;
      y += len;
    }
    else if (op == 'S' || op == 'H')
    {
      if (y <= pos && pos < y + len)
        return -1;
      y += len;
    }
  }
  return -1;
}

bool Postalt::run(std::vector<std::string>& inputs, std::string& outputs) 
{
  auto is_alt = p_alt->get_is_alt();
  auto idx_alt = p_alt->get_idx_alt();

  std::vector<std::vector<std::string>> buf2;
  // Process SAM
  for (auto& line : inputs)
  {
    // Print and skip the header line
    if (line.at(0) == '@')
    {
      outputs += line;
      continue;
    }

    std::vector<std::string> vec_s = split_str(line, '\t');
    int flag = std::stoi(vec_s[1]);
    
    // Print bufferred reads
    // c0 means read1 or read2
    if (buf2.size() > 0 && (buf2[0][0] != vec_s[0] || (std::stoi(buf2[0][1]) & 0xc0) 
      != (flag & 0xc0)))
    {
      print_buffer(buf2, outputs);
      buf2.clear();
    }

    // Skip unmapped lines
    // 4 means unmap
    if (flag & 4)
    {
      buf2.push_back(std::move(vec_s));
      continue;
    }

    // Parse the reported hit
    int NM = [&](){
      std::regex re(R"(\tNM:i:(\d+))");
      std::smatch ma;
      if (std::regex_search(line, ma, re))
        return std::stoi(ma[1]);
      else
        return 0;
    }();
    // 16 means reverse
    auto h = parse_hit(vec_s[2], (flag & 16), std::stoi(vec_s[3]), vec_s[5], NM);
    /// TODO: collect hla hits

    // The following does not work with hard clipped alignments
    if (h.hard)
    {
      buf2.push_back(std::move(vec_s));
      continue;
    }
    std::vector<Hit> hits{1, h};

    // Parse hits in the XA tag e.g. XA:Z:chr1,-119614933,100M,1;
    std::regex xa_regex(R"(\tXA:Z:(\S+))");
    std::smatch xa_match;
    if (std::regex_search(line, xa_match, xa_regex))
    {
      auto xa_strs = split_str(xa_match[1], ';');
      for (auto& s : xa_strs)
        hits.emplace_back(parse_hit(s));
    }

    // Check if there are ALT hits
    bool has_alt = false;
    for (auto& hit : hits)
    {
      if (is_alt.count(hit.ctg) != 0)
      {
        has_alt = true;
        break;
      }
    }
    if (!has_alt)
    {
      buf2.push_back(std::move(vec_s));
      continue;
    }

    // Lift mapping positions to the primary assembly
    int n_rpt_lifted = 0;
    Lift *rpt_lifted = nullptr;
    for (size_t i = 0; i < hits.size(); ++i)
    {
      auto& h = hits[i];
      auto iter = idx_alt.find(h.ctg);
      if (iter == idx_alt.end()) continue;
      auto a = (iter->second)(h.start, h.end);
      if (a.size() == 0) continue;

      // Find the approximate position on the primary assembly
      std::vector<Lift> lifted;
      for (size_t j = 0; j < a.size(); ++j)
      {
        auto& intv_alt = a[j];
        int s, e;
        if (!intv_alt.reverse)
        {
          // ALT is mapped to the forward strand of the primary assembly
          s = cigar2pos(intv_alt.cigar, h.start);
          e = cigar2pos(intv_alt.cigar, h.end - 1) + 1;
        }
        else
        {
          s = cigar2pos(intv_alt.cigar, intv_alt.len - h.end);
          e = cigar2pos(intv_alt.cigar, intv_alt.len - h.start - 1) + 1;
        }
        if (s < 0 || e < 0) continue; // read is mapped to clippings in the ALT-to-chr alignment
        s += intv_alt.seq_end;
        e += intv_alt.seq_end;
        lifted.emplace_back(intv_alt.contig, (h.rev != intv_alt.reverse), s, e);
        if (i == 0)
          ++n_rpt_lifted;
      }
      if (i == 0 && n_rpt_lifted == 1)
      {
        rpt_lifted = new Lift();
        *rpt_lifted = lifted[0];
        // memcpy(rpt_lifted, &(lifted[0]), sizeof(Lift));
      }
      if (lifted.size() > 0)
        hits[i].lifted = std::move(lifted);
    }


    // Release memory
    if (rpt_lifted != nullptr)
      delete rpt_lifted;
  }

  
  return true;
}

void Postalt::test_io() const
{
  std::string line;
  // while (std::cin >> line)
  // while (std::getline(std::cin, line))
  {
    //std::cout<<line<<std::endl;
  }

  //FILE* f = fopen(stdin);
  // char* buff = (char*)malloc(1024);
  constexpr int size = 1024;
  char buff[size];
  while (NULL != fgets(buff, size, stdin))
  {
    std::cout<<buff;
    // printf("%s", buff);
  }
  // free(buff);
  // fclose(f);

  return;
}