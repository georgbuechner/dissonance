#ifndef SRC_UTILS_H_
#define SRC_UTILS_H_

#include <chrono>

namespace utils {
  double get_elapsed(std::chrono::time_point<std::chrono::steady_clock> start,
    std::chrono::time_point<std::chrono::steady_clock> end);
}

#endif
