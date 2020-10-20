
#pragma once

#include "postalt/altfile.h"

#include <string>
#include <vector>

namespace postalt {

  /**
   * @brief Options of class Postalt
   */
  struct Option
  {
    int a;
    int b;
    int o;
    int e;
    int min_mapq;
    int min_sc;
    int max_nm_sc;
    float min_pa_ratio;
    Option() : a(1), b(4), o(6), e(1), min_mapq(10), min_sc(90),
      max_nm_sc(10), min_pa_ratio(1.0) {}
  };

   /**
   * @brief Structure of lifted
   */
  struct Lift
  {
    std::string contig;
    bool reverse;
    int start;
    int end;
    std::string lifted_str;
    Lift() {}
    Lift(std::string ctg, bool rev, int s, int e):
      contig(ctg), reverse(rev), start(s), end(e) {}
  };

  /**
   * @brief Structure of hit
   */
  struct Hit
  {
    std::string ctg;
    int start;
    bool rev;
    std::string cigar;
    int nm;
    bool hard;
    int end;
    int score;
    int l_query;
    std::vector<Lift> lifted;
    int i;  ///< the original index
    std::string pctg;
    int pstart;
    int pend;
    int g;
    std::string lifted_str;
  };

  /**
   * @brief A class for processing alt data for sam file
   */
  class Postalt {
   
  public:
    /**
     * @brief Creates a new postalt
     */
    Postalt();

    /**
     * @brief Creates a new postalt
     * @param alt_filename the name to alt file
     */
    Postalt(std::string alt_filename);

    /**
     * @brief Run the main progress
     * @param inputs sam records line by line
     * @param outputs a big string
     * @return true if everything ok
     */
    bool run(std::vector<std::string>& inputs, std::string& outputs);

    /**
     * @brief Read from stdin and write to stdout
     */
    void test_io() const;

  private:
    void print_buffer(std::vector<std::vector<std::string>>& buff, std::string& out);

    /**
     * @brief Parse a hit
     * @details inputs look like ["chr1", true, 1234, "100M", 5]
     * @return an object keeping various information about the alignment
     */
    auto parse_hit(std::string& contig, bool reverse, int start, std::string& cigar,
  int NM);

    /**
     * @brief Parse a hit
     * @param xa_str look like "chr10,-37885232,5S95M,3"
     * @return an object keeping various information about the alignment
     */
    auto parse_hit(std::string& xa_str);

    /**
     * @brief Given a pos on ALT and the ALT-to-REF CIGAR, find the pos on REF
     * @param cigar cigar list e.g. [('M',10), ('S', 12)]
     * @param pos position on ALT
     * @return position on REF
     */
    int cigar2pos(std::vector<std::pair<char, int>>& cigar, int pos);

    /**
     * @brief Reverse complement a DNA string
     */
    void revcomp(std::string& s);

    /** 
     * @brief Reverse a string
     */
    void reverse(std::string& s);

  private:
    std::string m_alt_filename; ///< input alt file

    Option opt;

    Altfile *p_alt;

    char m_rctab[256];  ///< reverse complement table
  };

}  // namespace postalt
