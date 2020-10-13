
#pragma once

#include <string>

namespace postalt {

  /**
   * @brief A class for processing alt data for sam file
   */
  class Postalt {
   
  public:
    /**
     * @brief Creates a new postalt
     * @param alt_filename the name to alt file
     */
    Postalt(std::string alt_filename);

    /**
     * @brief Run the main progress
     * @return true if everything ok
     */
    bool run() const;

    /**
     * @brief Read from stdin and write to stdout
     */
    void test_io() const;

  private:
    std::string m_alt_filename; ///< input alt file
    int m_block_lines;  ///< the max line numbers of a single block

  };

}  // namespace postalt
