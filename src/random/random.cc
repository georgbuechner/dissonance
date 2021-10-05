#include "random/random.h"
#include "audio/audio.h"
#include "constants/codes.h"
#include "spdlog/spdlog.h"
#include <cstddef>
#include <cstdlib>

#define LOGGER "logger"

RandomGenerator::RandomGenerator() {
  get_ran_ = &RandomGenerator::ran;
  last_point_ = 0;
}

RandomGenerator::RandomGenerator(AudioData analysed_data, size_t(RandomGenerator::*generator)(size_t, size_t)) {
  analysed_data_ = analysed_data;
  get_ran_ = generator;
  last_point_ = 0;
}

int RandomGenerator::RandomInt(size_t min, size_t max) {
  unsigned int random_faktor = (this->*get_ran_)(min, max);
  spdlog::get(LOGGER)->info("RandomGenerator::RandomInt: retuning {} < {} < {}", min, random_faktor, max);
  return random_faktor;
}

size_t RandomGenerator::ran(size_t min, size_t max) {
  return min + (rand()% (max - min + 1)); 
}

size_t RandomGenerator::ran_note(size_t min, size_t max) {
  auto data_at_beat = GetNextTimePointWithNotes(); 
  unsigned int random_faktor = data_at_beat.notes_.front().midi_note_;
  // Multiply random midi note by 1. bpm, 2. level if note remains below max.
  if (max > random_faktor) 
    random_faktor *= data_at_beat.bpm_;
  if (max > random_faktor)
    random_faktor += data_at_beat.level_;
  return min + (random_faktor % (max - min + 1)); 
}

size_t RandomGenerator::ran_boolean_minor_interval(size_t min, size_t max) {
  auto data_at_beat = GetNextTimePointWithNotes(); 
  auto intervals = Audio::GetInterval(data_at_beat.notes_);
  for (const auto& it : {MINOR_THIRD, MINOR_SEVENTH}) {
    if (std::find(intervals.begin(), intervals.end(), it) != intervals.end())
      return 1;
  }
  return 0;
}

size_t RandomGenerator::ran_level_based(size_t min, size_t max) {
  if (last_point_ == analysed_data_.data_per_beat_.size())
    last_point_ = 0;
  auto it = analysed_data_.data_per_beat_.begin();
  std::advance(it, last_point_++);
  return 999 - analysed_data_.average_level_-it->level_;
}

AudioDataTimePoint RandomGenerator::GetNextTimePointWithNotes() {
  if (last_point_ == analysed_data_.data_per_beat_.size())
    last_point_ = 0;
  auto it = analysed_data_.data_per_beat_.begin();
  std::advance(it, last_point_++);
  if (it->notes_.size() == 0)
    return GetNextTimePointWithNotes();
  return *it;
}
