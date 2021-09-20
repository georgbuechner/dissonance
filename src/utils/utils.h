#ifndef SRC_UTILS_H_
#define SRC_UTILS_H_

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

  std::string PositionToString(Position pos);

  int getrandom_int(int min, int max);
  
  Paragraphs LoadWelcome();
  Paragraphs LoadHelp();

  std::string create_id(std::string type);

  Options CreateOptionsFromStrings(std::vector<std::string> option_strings);
}

#endif
