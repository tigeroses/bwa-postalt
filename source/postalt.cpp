
#include <postalt/postalt.h>
#include <postalt/utility.h>

#include <iostream>
#include <unordered_map>

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

  // Construct reverse complement table
  for (int i = 0; i < 256; ++i)
    m_rctab[i] = 0;
  std::string s1 = "WSATUGCYRKMBDHVNwsatugcyrkmbdhvn";
  std::string s2 = "WSTAACGRYMKVHDBNwstaacgrymkvhdbn";
  for (size_t i = 0; i < s1.size(); ++i)
    m_rctab[static_cast<unsigned char>(s1.at(i))] = s2.at(i);
}

void Postalt::print_buffer(std::vector<std::vector<std::string>>& buff, std::string& out)
{
  if (buff.size() == 0) return;
  for (auto& b : buff)
    out += cat_str(b, '\t', true);
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
 
  auto cigar_pairs = parse_cigar(cigar);
  for (auto& [l, c] : cigar_pairs)
  {
    if (c == 'M') l_match += l;
    else if (c == 'D') ++n_del, l_del += l;
		else if (c == 'I') ++n_ins, l_ins += l;
		else if (c == 'N') l_skip += l;
		else if (c == 'H' || c == 'S') 
    {
			l_clip += l;
			if (c == 'H') h.hard = true;
		}
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
  std::vector<std::string> vec_s;
  split_str(xa_str, vec_s, ',');
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

  std::vector<std::string> xa_strs;
  std::vector<std::string> vec_s;

  // Process SAM
  int cnt = 0;
  for (auto& line : inputs)
  {
    ++cnt;
    if ((cnt % 100000) == 0)
      std::cerr<<"processed records: "<<cnt<<std::endl;

    // Print and skip the header line
    if (line.at(0) == '@')
    {
      outputs += line;
      continue;
    }

    // Remove the endline
    line.pop_back();
    
    split_str(line, vec_s, '\t');
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
    int NM = 0;
    for (size_t j = 11; j < vec_s.size(); ++j)
    {
      if (vec_s[j].substr(0,5) == "NM:i:")
      {
        NM = std::stoi(vec_s[j].substr(5));
        break;
      }
    }
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
    for (size_t j = 11; j < vec_s.size(); ++j)
    {
      if (vec_s[j].substr(0,5) == "XA:Z:")
      {
        split_str(vec_s[j].substr(5), xa_strs, ';');
        for (auto& s : xa_strs)
          hits.emplace_back(parse_hit(s));
        break;
      }
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

    // Prepare for hits grouping
    for (size_t i = 0; i < hits.size(); ++i)
    {
      // Set keys for sorting
      if (!hits[i].lifted.empty())
      {
        hits[i].pctg = hits[i].lifted[0].contig;
        hits[i].pstart = hits[i].lifted[0].start;
        hits[i].pend = hits[i].lifted[0].end;
      }
      else
      {
        hits[i].pctg = hits[i].ctg;
        hits[i].pstart = hits[i].start;
        hits[i].pend = hits[i].end;
      }
      hits[i].i = i;  // keep the original index
    }

    // Group hits based on the Lifted positions on non-ALT sequences
    if (hits.size() > 1)
    {
      std::sort(hits.begin(), hits.end(), [&](Hit& a, Hit& b){
        return a.pctg != b.pctg ? a.pctg < b.pctg : a.pstart < b.pstart;
      });
      std::string last_chr;
      int end = 0, g = -1;
      for (auto& h : hits)
      {
        if (last_chr != h.pctg)
        {
          ++g;
          last_chr = h.pctg;
          end = 0;
        }
        else if (h.pstart >= end)
          ++g;
        h.g = g;
        end = std::max(end, h.pend);
      }
    }
    else
    {
      hits[0].g = 0;
    }

    // Find the index and group id of the reported hit
    // find the size of the reported group
    int reported_g = 0, reported_i = 0, n_group0 = 0;
    if (hits.size() > 1)
    {
      for (size_t i = 0; i < hits.size(); ++i)
      {
        if (hits[i].i == 0)
        {
          reported_g = hits[i].g;
          reported_i = i;
        }
      }
      for (auto& h : hits)
        if (h.g == reported_g)
          ++n_group0;
    }
    else
    {
      if (is_alt.count(hits[0].ctg) == 0)
      {
        // No need the go through the following if the single hit is non-ALT
        buf2.push_back(std::move(vec_s));
        continue;
      }
      reported_g = reported_i = 0;
      n_group0 = 1;
    }

    // Re-estimate mapping quality if necessary
    int mapQ, ori_mapQ = std::stoi(vec_s[4]);
    // std::cerr<<"n_group0: "<<n_group0<<std::endl;
    if (n_group0 > 1)
    {
      std::unordered_map<int, int> tmp;
      for (auto& h : hits)
      {
        int g = h.g;
        if (tmp.count(g) == 0 || tmp[g] < h.score)
          tmp[g] = h.score;
      }
      std::vector<std::pair<int,int>> group_max;
      for (auto& [k,v] : tmp)
      {
        group_max.push_back({k,v});
        // std::cerr<<k<<" "<<v<<std::endl;
      }
      if (group_max.size() > 1)
        // Sort by score descending
        std::sort(group_max.begin(), group_max.end(), [&](auto& a, auto& b){
          return a.second > b.second;
        });
      // std::cerr<<group_max[0].first<<" "<<reported_g<<std::endl;
      if (group_max[0].first == reported_g)
      {
        // The best hit is the hit reported in SAM
        mapQ = group_max.size() == 1 ? 60 : 6 * (group_max[0].second - group_max[1].second);
      }
      else
        mapQ = 0;
      mapQ = std::min(mapQ, 60);
      if (idx_alt.count(vec_s[2]) == 0)
        mapQ = std::min(mapQ, ori_mapQ);
      else
        mapQ = std::max(mapQ, ori_mapQ);
    }
    else
      mapQ = ori_mapQ;

    // std::cerr<<"mapq: "<<mapQ<<std::endl;

    /// TODO: Find out whether the read is overlapping HLA genes

    // Adjust the mapQ of the primary hits
    if (n_rpt_lifted <= 1)
    {
      Lift* l = n_rpt_lifted == 1 ? rpt_lifted : nullptr;
      for (auto& s : buf2)
      {
        bool is_ovlp = true;
        if (l != nullptr)
        {
          if (l->contig != s[2]) // different chr
            is_ovlp = false;
          else if (((std::stoi(s[1]) & 16) != 0) != l->reverse) // different strand
            is_ovlp = false;
          else
          {
            int start = std::stoi(s[3]) - 1, end = start;
            auto cigar_pairs = parse_cigar(vec_s[5]);
            for (auto& [len, c] : cigar_pairs)
            {
              if (c == 'M' || c == 'D' || c == 'N')
                end += len;
            }
            if (!(start < l->end && l->start < end)) // no overlap
              is_ovlp = false;
          }
        }
        else
          is_ovlp = false;
        // Get the "pa" tag if present
        int om = -1;
        float pa = 10.0;
        for (size_t j = 11; j < s.size(); ++j)
        {
          if (vec_s[j].substr(0,5) == "om:i:")
          {
            om = std::stoi(vec_s[j].substr(5));
          }
          else if (vec_s[j].substr(0,5) == "pa:f:")
          {
            pa = std::stof(vec_s[j].substr(5));
          }
        }
        if (is_ovlp)
        {
          // overlapping the lifted hit
          int tmp = std::stoi(s[4]);
          if (om > 0)
            tmp = om;
          tmp = std::min(tmp, mapQ);
          s[4] = std::to_string(tmp);
        }
        else if (pa < opt.min_pa_ratio)
        {
          // not overlapping; has a small pa
          if (om < 0)
            s.push_back("om:i:" + s[4]);
          s[4] = "0";
        }
      }
    }

    // Generate lifted_str
    for (auto& h : hits)
    {
      if (h.lifted.empty()) continue;
      std::string u;
      for (auto& lifted : h.lifted)
        u += lifted.contig + "," + std::to_string(lifted.start) + "," +
          std::to_string(lifted.end) + "," + (lifted.reverse ? "-" : "+") + ";";
      h.lifted_str = u;
    }

    // Stage the reported hit
    vec_s[4] = std::to_string(mapQ);
    if (n_group0 > 1)
      vec_s.push_back("om:i:" + std::to_string(ori_mapQ));
    if (!hits[reported_i].lifted_str.empty())
      vec_s.push_back("lt:Z:" + hits[reported_i].lifted_str);
    buf2.push_back(vec_s);

    // Stage the hits generated from the XA tag
    std::string rs; // reverse quality
    std::string rq; // reverse complement sequence
    std::string rg;
    // std::smatch ma;
    for (size_t j = 11; j < vec_s.size(); ++j)
    {
      if (vec_s[j].substr(0,5) == "RG:Z:")
      {
        rg = vec_s[j];
        break;
      }
    }
    
    for (int i = 0; i < static_cast<int>(hits.size()); ++i)
    {
      auto& h = hits[i];
      if (h.g != reported_g || i == reported_i) continue;
      if (idx_alt.count(h.ctg) == 0) continue;
      std::vector<std::string> s{vec_s[0], "0", h.ctg, std::to_string(h.start+1),
        std::to_string(mapQ), h.cigar, vec_s[6], vec_s[7], vec_s[8]};
      if (vec_s[6] == "=" && s[2] != vec_s[2]) 
        s[6] = vec_s[2];
      // Print sequence/quality and set the rev flag
      if (h.rev == hits[reported_i].rev)
      {
        s.push_back(vec_s[9]);
        s.push_back(vec_s[10]);
        s[1] = std::to_string(flag | 0x800);
      }
      else
      {
        // We need to write the reverse sequence
        if (rs.empty() || rq.empty())
        {
          rs = vec_s[9];
          revcomp(rs);
          rq = vec_s[10];
          reverse(rq);
        }
        s.push_back(rs);
        s.push_back(rq);
        s[1] = std::to_string((flag ^ 0x10) | 0x800);
      }
      s.push_back("NM:i:" + std::to_string(h.nm));
      if (!h.lifted_str.empty())
        s.push_back("lt:Z:" + h.lifted_str);
      if (!rg.empty())
        s.push_back(rg);
      buf2.push_back(std::move(s));
    }

    // Release memory
    if (rpt_lifted != nullptr)
      delete rpt_lifted;
  }
  print_buffer(buf2, outputs);

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

void Postalt::revcomp(std::string& s)
{
  reverse(s);
  std::for_each(s.begin(), s.end(), [&](char& c){
    c = m_rctab[static_cast<unsigned char>(c)];
  });
}

void Postalt::reverse(std::string& s)
{
  std::reverse(s.begin(), s.end());
}