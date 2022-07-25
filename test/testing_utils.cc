#include "testing_utils.h"

position_t t_utils::GetRandomPositionInField(Field* field, RandomGenerator* ran_gen) {
  return {ran_gen->RandomInt(5, field->lines()-5), ran_gen->RandomInt(5, field->cols()-5)};
}


