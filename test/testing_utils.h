#ifndef TEST_TESTING_UTILS_H_
#define TEST_TESTING_UTILS_H_

#include "server/game/field/field.h"
#include "share/tools/random/random.h"

namespace t_utils {
  position_t GetRandomPositionInField(std::shared_ptr<Field> field, std::shared_ptr<RandomGenerator> ran_gen);
}

#endif
