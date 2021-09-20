#include <atomic>
#include <algorithm>
#include <cstddef>
#include <fstream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include "audio/audio.h"
#include "constants/codes.h"
#include "spdlog/spdlog.h"

#define LOGGER "logger"


std::atomic<bool> pause_audio(false);

const std::vector<std::string> Audio::note_names_ = {
  "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B"
};
std::map<std::string, std::vector<std::string>> Audio::keys_ = {};


Audio::Audio() {}

// getter 
AudioData& Audio::analysed_data() {
  return analysed_data_;
}

std::map<std::string, std::vector<std::string>> Audio::keys() {
  return keys_;
}

// setter 
void Audio::set_source_path(std::string source_path) {
  source_path_ = source_path;
}

void Audio::Analyze() {
  spdlog::get(LOGGER)->debug("Audio::Analyze: starting analyses. Starting audi-data extraction");
  std::list<AudioDataTimePoint> data_per_beat;

  uint_t samplerate = 0;
  uint_t win_size = 1024; // window size
  uint_t hop_size = win_size / 4;
  uint_t n_frames = 0, read = 0;

  // Load audio-file
  aubio_source_t * source = new_aubio_source(source_path_.c_str(), samplerate, hop_size);
  if (!source) { 
    aubio_cleanup();
    return;
  }
  // Update samplerate.
  samplerate = aubio_source_get_samplerate(source);

  // create some vectors
  fvec_t * in = new_fvec (hop_size); // input audio buffer
  fvec_t * out = new_fvec (1); // output position
  fvec_t * out_notes = new_fvec (1); // output position
  
  // create tempo- and notes-object
  aubio_tempo_t * bpm_obj = new_aubio_tempo("default", win_size, hop_size, samplerate);
  aubio_notes_t * notes_obj = new_aubio_notes ("default", win_size, hop_size, samplerate);
  if (!bpm_obj && !notes_obj) { 
    del_fvec(in);
    del_fvec(out);
    del_aubio_source(source);
    return;
  }

  float average_bpm = 0.0f;
  float average_level = 0.0f;
  std::vector<Note> last_notes;
  do {
    // Put some fresh data in input vector
    aubio_source_do(source, in, &read);
    // execute tempo and notes, add notes to last notes.
    aubio_tempo_do(bpm_obj,in,out);
    aubio_notes_do(notes_obj, in, out_notes);
    if (out_notes->data[0] != 0)
      last_notes.push_back(ConvertMidiToNote(out_notes->data[0]));

    // do something with the beats
    if (out->data[0] != 0) {
      // Get current level and bpm
      int level = 100-(-1*aubio_level_detection(in, -90.));
      int bpm = aubio_tempo_get_bpm(bpm_obj);
      average_bpm += bpm;
      average_level += level;
      // Add data-point and clear last notes.
      data_per_beat.push_back(AudioDataTimePoint({aubio_tempo_get_last_ms(bpm_obj), bpm, level, last_notes}));
      last_notes.clear();
    }
    n_frames += read;
  } while ( read == hop_size );

  // clean up memory
  del_aubio_tempo(bpm_obj);
  del_fvec(in);
  del_fvec(out);
  del_aubio_source(source);
  aubio_cleanup();

  spdlog::get(LOGGER)->debug("Audio::Analyze: got all data. Analyzing extracted data.");

  average_bpm /= data_per_beat.size();
  average_level /= data_per_beat.size();

  // Create analysed_data.
  analysed_data_ = AudioData({data_per_beat, average_bpm, average_level});
  // Add information on keys
  CreateLevels(8);
  Safe();
}

void Audio::Safe() {
  nlohmann::json data = {{"average_bpm", analysed_data_.average_bpm_}, {"average_level", analysed_data_.average_level_}};
  data["bpms"] = nlohmann::json::array();
  data["levels"] = nlohmann::json::array();
  data["notes"] = nlohmann::json::array();
  for (const auto& it : analysed_data_.data_per_beat_) {
    data["bpms"].push_back(it.bpm_);
    data["levels"].push_back(it.level_);
    std::vector<int> midis;
    for (const auto& note : it.notes_)
      midis.push_back(note.midi_note_);
    data["notes"].push_back(midis);
  }
  std::string out_path = source_path_;
  out_path.replace(out_path.length()-3, out_path.length(), "json");
  out_path.replace(5, 5, "analysis");

  std::ofstream write(out_path.c_str());
  if (!write)
    spdlog::get(LOGGER)->error("Audio::Safe: Could not safe at {}", out_path);
  else {
    spdlog::get(LOGGER)->info("Audio::Safe: safeing at {}", out_path);
    write << data;
  }
  write.close();
}

void Audio::play() {
  spdlog::get(LOGGER)->debug("Audio::play");
  ma_result result;
  ma_device_config deviceConfig;

  result = ma_decoder_init_file(source_path_.c_str(), NULL, &decoder_);
  if (result != MA_SUCCESS) {
    spdlog::get(LOGGER)->debug("Audio::play: Failed to load audio");
    return;
  }

  deviceConfig = ma_device_config_init(ma_device_type_playback);
  deviceConfig.playback.format   = decoder_.outputFormat;
  deviceConfig.playback.channels = decoder_.outputChannels;
  deviceConfig.sampleRate        = decoder_.outputSampleRate;
  deviceConfig.dataCallback      = data_callback;
  deviceConfig.pUserData         = &decoder_;

  if (ma_device_init(NULL, &deviceConfig, &device_) != MA_SUCCESS) {
    spdlog::get(LOGGER)->debug("Audio::play: Failed to open playback device.");
    ma_decoder_uninit(&decoder_);
    return;
  }

  if (ma_device_start(&device_) != MA_SUCCESS) {
    spdlog::get(LOGGER)->debug("Audio::play: Failed to start playback device.");
    ma_device_uninit(&device_);
    ma_decoder_uninit(&decoder_);
    return;
  }
  return;
}

void Audio::Pause() {
  pause_audio = true;
}

void Audio::Unpause() {
  pause_audio = false;
}

void Audio::Stop() {
  ma_device_uninit(&device_);
  ma_decoder_uninit(&decoder_);
}

void Audio::data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
  ma_decoder* pDecoder = (ma_decoder*)pDevice->pUserData;
  if (pDecoder == NULL)
    return;
  if (!pause_audio) {
    ma_decoder_read_pcm_frames(pDecoder, pOutput, frameCount);
    (void)pInput;
  }
}

Note Audio::ConvertMidiToNote(int midi_note) {
  Note note = Note();
  note.midi_note_ = midi_note;
  note.note_name_ = note_names_[(midi_note-24)%12];
  note.note_ = (midi_note-24)%12;
  note.ocatve_ = (midi_note-12)/12;
  return note;
}

void Audio::CreateKeys() {
  spdlog::get(LOGGER)->debug("Audio::CreateKeys");
  std::map<std::string, std::vector<std::string>> keys;
  for (size_t i=0; i<note_names_.size(); i++) {
    // Construct minor keys:
    std::vector<std::string> notes_minor;
    for (const auto& step : {0, 2, 4, 5, 7, 9, 11})
      notes_minor.push_back(note_names_[(i+step)%12]);
    keys[note_names_[i] + "Minor"] = notes_minor;
    // Construct major keys:
    std::vector<std::string> notes_major;
    for (const auto& step : {0, 2, 3, 5, 7, 8, 10})
      notes_major.push_back(note_names_[(i+step)%12]);
    keys[note_names_[i] + "Major"] = notes_major;
  }
  keys_ = keys;
}

void Audio::CreateLevels(int intervals) {
  spdlog::get(LOGGER)->debug("Audio::CreateLevels");
  // 1. Sort notes by frequency:
  std::map<std::string, int> notes_by_frequency;
  long unsigned int counter = 0;
  size_t cur_interval = 0;
  size_t darkness = 0;
  size_t total = 0;
  for (auto& it : analysed_data_.data_per_beat_) {
    for (const auto& note : it.notes_) {
      notes_by_frequency[note.note_name_]++;
      darkness += note.ocatve_*note.ocatve_;
      total+=note.ocatve_;
    }
    // If next intervals was reached, calc key and increase current level.
    if (++counter > analysed_data_.data_per_beat_.size()/intervals*(cur_interval+1)) {
      it.interval_ = cur_interval;
      darkness /= total;
      CalcLevel(cur_interval++, notes_by_frequency, darkness);
      notes_by_frequency.clear();
      darkness = 0;
      total = 0;
    }
  }
}

void Audio::CalcLevel(size_t interval, std::map<std::string, int> notes_by_frequency, size_t darkness) {
  spdlog::get(LOGGER)->debug("Audio::CalcLevel");
  std::list<std::pair<int, std::string>> sorted_notes_by_frequency;
  // Transfor to ordered list
  for (const auto& it : notes_by_frequency)
    sorted_notes_by_frequency.push_back({it.second, it.first});
  sorted_notes_by_frequency.sort();
  sorted_notes_by_frequency.reverse();

  // Get note with highest frequency.
  std::string key = sorted_notes_by_frequency.front().second; 
  auto it = std::find(note_names_.begin(), note_names_.end(), key);
  size_t key_note = it - note_names_.begin();

  // Check minor/ major
  size_t notes_in_minor = 0;
  size_t notes_in_major = 0;
  std::vector<std::string> notes = keys_.at(key + "Major");
  for (const auto& it : sorted_notes_by_frequency)
    if (std::find(notes.begin(), notes.end(), it.second) != notes.end())
      notes_in_major += it.first;
  notes = keys_.at(key + "Minor");
  for (const auto& it : sorted_notes_by_frequency)
    if (std::find(notes.begin(), notes.end(), it.second) != notes.end())
      notes_in_minor += it.first;
  key += (notes_in_minor > notes_in_major) ? "Minor" : "Major";

  // Calculate number of notes inside and outside of key.
  size_t notes_in_key = 0;
  notes = keys_.at(key);
  for (const auto& it : sorted_notes_by_frequency)
     if (std::find(notes.begin(), notes.end(), it.second) != notes.end())
      notes_in_key++;

  // Add new interval information.
  analysed_data_.intervals_[interval] = Interval({interval, key, key_note, 
      Signitue::UNSIGNED, key.find("Major") != std::string::npos, notes_in_key, 
      sorted_notes_by_frequency.size()-notes_in_key, darkness}
    );
  if (key.find("#") != std::string::npos)
    analysed_data_.intervals_[interval].signature_ = Signitue::SHARP;
  else if (key.find("b") != std::string::npos)
    analysed_data_.intervals_[interval].signature_ = Signitue::FLAT;
}

bool Audio::MoreOfNotes(const AudioDataTimePoint &data_at_beat, bool off) const {
  std::string cur_key = analysed_data_.intervals_.at(data_at_beat.interval_).key_;
  auto notes_in_cur_key = keys_.at(cur_key);
  size_t off_notes_counter = 0;
  for (const auto& note : data_at_beat.notes_) {
    if (off) {
      if (std::find(notes_in_cur_key.begin(), notes_in_cur_key.end(), note.note_name_) == notes_in_cur_key.end())
        off_notes_counter++;
    }
    else {
      if (std::find(notes_in_cur_key.begin(), notes_in_cur_key.end(), note.note_name_) != notes_in_cur_key.end())
        off_notes_counter++;
    }
  }
  return off_notes_counter == data_at_beat.notes_.size() && off_notes_counter > 0;
}

size_t Audio::NextOfNotesIn(double cur_time) const {
  size_t counter = 1;
  for (const auto& it : analysed_data_.data_per_beat_) {
    if (it.time_ <= cur_time) 
      continue;
    if (MoreOfNotes(it))
      break;
    counter++;
  }
  return counter;
}
