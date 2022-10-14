#ifndef TEST_TESTING_UTILS_H_
#define TEST_TESTING_UTILS_H_

#include "server/game/field/field.h"
#include "share/objects/units.h"
#include "server/game/player/player.h"
#include "share/tools/random/random.h"
#include <memory>

namespace t_utils {
  position_t GetRandomPositionInField(std::shared_ptr<Field> field, std::shared_ptr<RandomGenerator> ran_gen);
}

#endif
