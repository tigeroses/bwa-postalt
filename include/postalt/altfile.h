
#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <functional>

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
     * @brief Create index for a list of intervals for fast interval queries
     * @param intv list of intervals
     * @param bits the bits number of shift right
     * @return closure function for interval queries
     */
    template<typename T>
    std::function<std::vector<T>(int,int)> intv_ovlp(std::vector<T>& intv, int bits = 13);

    template<typename T>
    std::function<std::vector<T>(int,int)> intv_ovlp(std::vector<T>& intv, int bits)
    {
        std::sort(intv.begin(), intv.end(), [&](T& a, T& b){
            return a.start < b.start;
        });
        // Create the index
        std::unordered_map<int, int> idx;
        int max = 0;
        for (int i = 0; i < static_cast<int>(intv.size()); ++i)
        {
            int b = (intv[i].start >> bits);
            int e = ((intv[i].end-1) >> bits);
            if (b != e)
            {
                for (int j = b; j <= e; ++j)
                {
                    if (idx.count(j) == 0)
                        idx[j] = i;
                }
            }
            else if (idx.count(b) == 0)
            {
                idx[b] = i;
            }
            max = std::max(max, e);
        }

        // closure
        auto f = [=](int _b, int _e)
        {
            std::vector<T> ovlp;

            int x = (_b >> bits);
            if (x > max) return ovlp;
            int off;
            if (idx.count(x) == 0)
            {
                int i;
                for (i = ((_e-1) >> bits) - 1; i >= 0; --i)
                {
                    if (idx.count(i) != 0) 
                        break;
                }
                x = i;
            }
            auto iter = idx.find(x);
            off = x < 0 ? 0 : iter->second;
            
            for (int i = off; i < static_cast<int>(intv.size()) && intv[i].start < _e; ++i)
            {
                if (intv[i].end > _b)
                    ovlp.push_back(intv[i]);
            }
            return ovlp;
        };
        return f;
    }

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
         * @note Deprecated
         */
        void create_overlap();

        /**
         * @brief Return unique set of alt names
         */
        std::unordered_set<std::string> get_is_alt();

        /**
         * @brief Return index of alt
         */
        std::unordered_map<std::string, std::function<std::vector<IntvAlt>(int, int)>> get_idx_alt();

        /**
         * @brief Return index of pri
         */
        std::unordered_map<std::string, std::function<std::vector<IntvPri>(int, int)>> get_idx_pri();
        
    private:
        std::string m_filename;

        std::unordered_set<std::string> m_is_alt;
        std::unordered_map<std::string, int> m_hla_ctg;
        std::string m_hla_chr;

        std::unordered_map<std::string, std::vector<IntvAlt>> m_intv_alt;
        std::unordered_map<std::string, std::vector<IntvPri>> m_intv_pri;

        std::unordered_map<std::string, std::function<std::vector<IntvAlt>(int, int)>> m_idx_alt;
        std::unordered_map<std::string, std::function<std::vector<IntvPri>(int, int)>> m_idx_pri;
    };
}
