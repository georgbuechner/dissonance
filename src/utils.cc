#include "utils.h"
#include <math.h>

double utils::get_elapsed(std::chrono::time_point<std::chrono::steady_clock> start,
    std::chrono::time_point<std::chrono::steady_clock> end) {
  return std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
}

int utils::dist(Position pos1, Position pos2) {
  return std::sqrt(pow(pos2.first - pos1.first, 2) + pow(pos2.second - pos1.second, 2));
}

std::string utils::PositionToString(Position pos) {
  return std::to_string(pos.first) + "|" + std::to_string(pos.second);
}

