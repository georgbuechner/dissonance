#ifndef SRC_RANDOM_RANDOM_H_
#define SRC_RANDOM_RANDOM_H_

#include <cstddef>

#include "share/audio/audio.h"

class RandomGenerator {
  public:
    /** 
     * Constructor for tests (no audio needed). Uses simple system random
     * function.
     */
    RandomGenerator();

    /**
     * Constructor with audio data and custom random function.
     * @param[in] analysed_data used for generating random numbers.
     * @param[in] generator custom function to generate random numbers based on
     * audio data.
     */
    RandomGenerator(AudioData analysed_data, size_t(RandomGenerator::*generator)(size_t, size_t));

    /**
     * Base function calling set random number generator.
     * @param[in] min
     * @param[in] maz
     * @return random number between min and max.
     */
    int RandomInt(size_t min, size_t max);

    // generators
    /**
     * Gets random number from std::random.
     */
    size_t ran(size_t min, size_t max);

    /**
     * Gets random number based on the first note at beat. Uses multiplication
     * with bpm and level, if base-factor is to small.
     * @param[in] min
     * @param[in] maz
     * @return random number between min and max.
     */
    size_t ran_note(size_t min, size_t max);

    /**
     * Gets random number based on the existance of a minor third intervals in
     * the last notes played. Only use for boolean values!
     * @param[in] min
     * @param[in] max
     * @return random number between min and max.
     */
    size_t ran_boolean_minor_interval(size_t min, size_t max);

    size_t ran_level_peaks(size_t min, size_t max);

  private:
    // member
    AudioData analysed_data_;
    size_t last_point_;
    std::vector<int> peaks_;
    size_t(RandomGenerator::*get_ran_)(size_t min, size_t max);

    // functions:

    /**
     * Gets next data_at_beat. Cycles through all data (if end is reached,
     * starts at the beginning again.). Skips time point if notes are empty.
     */
    AudioDataTimePoint GetNextTimePointWithNotes();
};

#endif
