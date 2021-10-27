#ifndef TEST_TESTING_UTILS_H_
#define TEST_TESTING_UTILS_H_

#include "game/field.h"
#include "objects/units.h"
#include "player/player.h"
#include "random/random.h"

namespace t_utils {
  position_t GetRandomPositionInField(Field* field, RandomGenerator* ran_gen);
}

#endif
