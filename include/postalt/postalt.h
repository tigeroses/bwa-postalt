
#pragma once

#include <string>

namespace postalt {

  /**
   * @brief A class for processing alt data for sam file
   */
  class Postalt {
    std::string alt_filename;

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
  };

}  // namespace postalt
