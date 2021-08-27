#ifndef SRC_UTILS_H_
#define SRC_UTILS_H_

#include <chrono>
#include <string>
#include <utility>
#include <vector>

namespace utils {

  typedef std::pair<int, int> Position;
  typedef std::vector<std::vector<std::string>> Paragraphs;

  double get_elapsed(std::chrono::time_point<std::chrono::steady_clock> start,
    std::chrono::time_point<std::chrono::steady_clock> end);

  double dist(Position pos1, Position pos2);

  std::string PositionToString(Position pos);

  int getrandom_int(int min, int max);
  
  Paragraphs LoadWelcome();
  Paragraphs LoadHelp();

  std::string create_id();
}

#endif
