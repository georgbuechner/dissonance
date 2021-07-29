#ifndef SRC_SOLDIER_H_
#define SRC_SOLDIER_H_

#include <chrono>
#include <list>

typedef std::pair<int, int> Position;

struct Unit {
  Position pos_;

  int speed_;  ///< lower number means higher speed.
  std::chrono::time_point<std::chrono::steady_clock> last_action_; 

  Unit() : pos_({}), speed_(999), last_action_(std::chrono::steady_clock::now()) {}
  Unit(Position pos, int speed) : pos_(pos), speed_(speed), last_action_(std::chrono::steady_clock::now()) {}
};

struct Soldier : Unit {
  std::list<Position> way_;

  Soldier() : Unit () {}
  Soldier(Position pos, std::list<Position> way) : Unit(pos, 500), way_(way) {}
};

struct DefenceTower : Unit{
  int range_;

  DefenceTower() : Unit() {}
  DefenceTower(Position pos) : Unit(pos, 800), range_(4) {}
};

struct Den : Unit {
  int lp_;

  Den() : Unit() {}
  Den(Position pos, int lp) : Unit(pos, 999), lp_(lp) {}
};

#endif
