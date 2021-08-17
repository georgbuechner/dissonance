#ifndef SRC_UTILS_H_
#define SRC_UTILS_H_

#include <chrono>
#include <string>
#include <utility>

namespace utils {

  typedef std::pair<int, int> Position;

  double get_elapsed(std::chrono::time_point<std::chrono::steady_clock> start,
    std::chrono::time_point<std::chrono::steady_clock> end);

  int dist(Position pos1, Position pos2);

  std::string PositionToString(Position pos);
}

#endif
