#ifndef SRC_UTILS_H_
#define SRC_UTILS_H_

#include "nlohmann/json.hpp"
#include "objects/data_structs.h"
#include <chrono>
#include <string>
#include <utility>
#include <vector>

namespace utils {

  typedef std::pair<int, int> Position;
  typedef std::vector<std::vector<std::string>> Paragraphs;

  /**
  * @param[in] str string to be splitet
  * @param[in] delimitter 
  * @return vector
  */
  std::vector<std::string> Split(std::string str, std::string delimiter);

  double get_elapsed(std::chrono::time_point<std::chrono::steady_clock> start,
    std::chrono::time_point<std::chrono::steady_clock> end);

  double dist(Position pos1, Position pos2);
  bool InRange(Position pos1, Position pos2, double min_dist, double max_dist);

  std::string PositionToString(Position pos);

  int getrandom_int(int min, int max);
  
  Paragraphs LoadWelcome();
  Paragraphs LoadHelp();

  std::string create_id(std::string type);

  Options CreateOptionsFromStrings(std::vector<std::string> option_strings);

  /** 
   * @brief Loads json from disc
   * @param[in] path path to json.
   * @return boolean indicating success.
   */
  nlohmann::json LoadJsonFromDisc(std::string path);

  /** 
   * @brief Loads json from disc
   * @param[in] path path to json.
   * @param[out] reference to json.
   * @return boolean indicating success.
   */
  void WriteJsonFromDisc(std::string path, nlohmann::json& json);
}

#endif
