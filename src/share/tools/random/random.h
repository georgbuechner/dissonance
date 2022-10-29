#ifndef SRC_RANDOM_RANDOM_H_
#define SRC_RANDOM_RANDOM_H_

#include "share/audio/audio.h"
#include <vector>

class RandomGenerator {
  public:
    /** 
     * Constructor for tests (no audio needed). Uses simple system random
     * function.
     */
    RandomGenerator();

    AudioData& analysed_data() { return analysed_data_; }

    /**
     * Constructor with audio data and custom random function.
     * @param[in] analysed_data used for generating random numbers.
     * @param[in] generator custom function to generate random numbers based on
     * audio data.
     */
    RandomGenerator(AudioData analysed_data, int(RandomGenerator::*generator)(int, int));

    /**
     * Base function calling set random number generator.
     * @param[in] min
     * @param[in] maz
     * @return random number between min and max.
     */
    int RandomInt(int min, int max);

    // generators

    /**
     * Gets random number based on the first note at beat. Uses multiplication
     * with bpm and level, if base-factor is to small.
     * @param[in] min
     * @param[in] maz
     * @return random number between min and max.
     */
    int ran_note(int min, int max);

  private:
    // member
    AudioData analysed_data_;
    int last_point_;
    std::vector<int> peaks_;
    int(RandomGenerator::*get_ran_)(int min, int max);

    // functions:

    /**
     * Gets next data_at_beat. Cycles through all data (if end is reached,
     * starts at the beginning again.). Skips time point if notes are empty.
     */
    AudioDataTimePoint GetNextTimePointWithNotes();

    /**
     * Gets random number from std::random.
     */
    int ran(int min, int max);

};

#endif
