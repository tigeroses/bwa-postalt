
#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>

namespace postalt 
{
    /**
     * @brief Structure for creating intervals
     */
    struct IntvAlt
    {
        int start;
        int end;
        int len;
        std::string contig;
        bool reverse;
        int seq_end;
        std::vector<std::pair<char, int>> cigar;
        int tlen;
    };

    /**
     * @brief Structure for creating intervals
     */
    struct IntvPri
    {
        int start;
        int end;
        std::string qname;
    };

    /**
     * @brief Read alt file and preprocess it
     */
    class Altfile 
    {
    public:
        /**
         * @brief Create a new Altfile
         */
        Altfile(std::string file);

        /**
         * @brief Read and parse the file
         */
        void parse();

        /**
         * @brief Create overlaps for searching
         */
        void create_overlap();

    private:
        std::string m_filename;

        std::unordered_set<std::string> m_is_alt;
        std::unordered_map<std::string, int> m_hla_ctg;
        std::string m_hla_chr;

        std::unordered_map<std::string, std::vector<IntvAlt>> m_intv_alt;
        std::unordered_map<std::string, std::vector<IntvPri>> m_intv_pri;
    };
}