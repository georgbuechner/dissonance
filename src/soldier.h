#ifndef SRC_SOLDIER_H_
#define SRC_SOLDIER_H_

#include <list>

struct Soldier {
  std::pair<int, int> cur_pos_;
  std::list<std::pair<int, int>> way_;
};

#endif
