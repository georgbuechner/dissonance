#include "share/audio/audio.h"
#include "share/defines.h"
#include "share/constants/codes.h"
#include "share/tools/utils/utils.h"

#include <atomic>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <nlohmann/json.hpp>
#include <string>
#include "spdlog/spdlog.h"

std::atomic<bool> pause_audio(false);

const std::vector<std::string> Audio::note_names_ = {
  "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B"
};
std::map<std::string, std::vector<std::string>> Audio::keys_ = {};


Audio::Audio(std::string base_path) : base_path_(base_path) {}

// getter 
AudioData& Audio::analysed_data() {
  return analysed_data_;
}

std::map<std::string, std::vector<std::string>> Audio::keys() {
  return keys_;
}

std::string Audio::filename(bool shortened) {
  std::string filename = filename_;
  if (shortened && filename.size() > 7)
    filename.erase(7);
  return filename;
}

// setter 
void Audio::set_source_path(std::string source_path) {
  std::filesystem::path p = source_path;
  filename_ = p.filename();
  source_path_ = source_path;
}

void Audio::Analyze() {
  spdlog::get(LOGGER)->debug("Audio::Analyze: starting analyses. Starting audi-data extraction");
  // Load or analyse data.
  std::string out_path = GetOutPath(source_path_);
  analysed_data_ = (std::filesystem::exists(out_path)) ? Load(out_path) : AnalyzeFile(source_path_);
  AnalyzePeak();
}

void Audio::Analyze(nlohmann::json data) {
  spdlog::get(LOGGER)->debug("Audio::Analyze: starting analyses. Starting audi-data extraction");
  // Load or analyse data.
  analysed_data_ = Load(data);
  AnalyzePeak();
}

nlohmann::json Audio::GetAnalyzedData() {
  std::string out_path = GetOutPath(source_path_);
  return utils::LoadJsonFromDisc(out_path);
}

void Audio::AnalyzePeak() {
  spdlog::get(LOGGER)->info("Analyzing max peak");
  int max = 0;
  for (const auto& it : analysed_data_.data_per_beat_) {
    int new_max = it.level_- analysed_data_.average_level_;
    if (new_max > max)
      max = new_max;
  }
  analysed_data_.max_peak_ = max;
  spdlog::get(LOGGER)->info("Done");

  // Create analysed_data.
  // Add information on keys
  CreateLevels(8);
}

AudioData Audio::AnalyzeFile(std::string source_path) {
  spdlog::get(LOGGER)->debug("Audio::AnalyzeFile: starting analyses of {}", source_path); 
  std::list<AudioDataTimePoint> data_per_beat;
  uint_t samplerate = 0;
  uint_t win_size = 1024; // window size
  uint_t hop_size = win_size / 4;
  uint_t n_frames = 0, read = 0;

  // Load audio-file
  aubio_source_t * source = new_aubio_source(source_path.c_str(), samplerate, hop_size);
  if (!source) { 
    aubio_cleanup();
    throw "Could not load audio-source";
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
    throw "Could not create notes or bpm object.";
  }

  float average_bpm = 0.0f;
  float average_level = 0.0f;
  std::vector<Note> last_notes;
  std::vector<int> last_levels;
  do {
    // Put some fresh data in input vector
    aubio_source_do(source, in, &read);
    // execute tempo and notes, add notes to last notes.
    aubio_tempo_do(bpm_obj,in,out);
    aubio_notes_do(notes_obj, in, out_notes);
    if (out_notes->data[0] != 0)
      last_notes.push_back(ConvertMidiToNote(out_notes->data[0]));
    last_levels.push_back(100-(-1*aubio_level_detection(in, -90.)));

    // do something with the beats
    if (out->data[0] != 0) {
      // Get current level and bpm
      int level = std::accumulate(last_levels.begin(), last_levels.end(), 0.0)/last_levels.size();
      last_levels.clear();
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
  auto analysed_data = AudioData({data_per_beat, average_bpm, average_level});
  Safe(analysed_data, source_path);
  return analysed_data;
}

void Audio::Safe(AudioData analysed_data, std::string source_path) {
  nlohmann::json data = {{"average_bpm", analysed_data.average_bpm_}, {"average_level", analysed_data.average_level_}};
  data["time_points"] = nlohmann::json::array();
  for (const auto& it : analysed_data.data_per_beat_) {
    std::vector<int> midis;
    for (const auto& note : it.notes_)
      midis.push_back(note.midi_note_);
    data["time_points"].push_back({{"time", it.time_}, {"bpm", it.bpm_}, {"level", it.level_}, {"notes", midis}});
  }
  utils::WriteJsonFromDisc(GetOutPath(source_path), data);
}

AudioData Audio::Load(std::string source_path) {
  nlohmann::json data = utils::LoadJsonFromDisc(source_path);
  return Load(data);
}

AudioData Audio::Load(nlohmann::json data) {
  AudioData audio_data;
  audio_data.average_bpm_ = data["average_bpm"];
  audio_data.average_level_ = data["average_level"];
  std::list<AudioDataTimePoint> data_per_beat;
  for (const auto& it : data["time_points"]) {
    std::vector<int> midis = it["notes"];
    std::vector<Note> notes;
    for (const auto& midi_note : midis) 
      notes.push_back(ConvertMidiToNote(midi_note));
    data_per_beat.push_back({it["time"], it["bpm"], it["level"], notes});
  }
  audio_data.data_per_beat_ = data_per_beat;
  return audio_data;
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
  note.note_ = (midi_note-24)%12;
  note.note_name_ = note_names_[note.note_];
  note.ocatve_ = (midi_note-12)/12;
  return note;
}

void Audio::Initialize() {
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
  spdlog::get(LOGGER)->debug("Created level with darkness: {}, now: {}", darkness, 
      analysed_data_.intervals_[interval].darkness_);
  if (key.find("#") != std::string::npos)
    analysed_data_.intervals_[interval].signature_ = Signitue::SHARP;
  else if (key.find("b") != std::string::npos)
    analysed_data_.intervals_[interval].signature_ = Signitue::FLAT;
}

bool Audio::MoreOffNotes(const AudioDataTimePoint &data_at_beat, bool off) const {
  spdlog::get(LOGGER)->debug("Audio::MoreOffNotes");
  if (analysed_data_.intervals_.count(data_at_beat.interval_) == 0) {
    spdlog::get(LOGGER)->error("Audio::MoreOffNotes: interval not in intervals! {}", data_at_beat.interval_);
    return false;
  }
  std::string cur_key = analysed_data_.intervals_.at(data_at_beat.interval_).key_;
  if (keys_.count(cur_key) == 0) {
    spdlog::get(LOGGER)->error("Audio::MoreOffNotes: key not in keys! {}", cur_key);
    return false;
  }
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
  spdlog::get(LOGGER)->info("Audio::MoreOffNotes: done");
  return off_notes_counter == data_at_beat.notes_.size() && off_notes_counter > 0;
}

size_t Audio::NextOfNotesIn(double cur_time) const {
  spdlog::get(LOGGER)->debug("Audio::NextOfNotesIn");
  size_t counter = 1;
  for (const auto& it : analysed_data_.data_per_beat_) {
    if (it.time_ <= cur_time) 
      continue;
    if (MoreOffNotes(it))
      break;
    counter++;
  }
  spdlog::get(LOGGER)->info("Audio::NextOfNotesIn: done");
  return counter;
}

std::string Audio::GetOutPath(std::filesystem::path source_path) {
  source_path.replace_extension(".json");
  std::hash<std::string> hasher;
  size_t hash = hasher(source_path);
  std::string out_path = base_path_ + "/data/analysis/" + std::to_string(hash) + source_path.filename().string();
  spdlog::get(LOGGER)->info("Audio::GetOutPath: got out_path: {}", out_path);
  return out_path;
}

std::map<unsigned short, std::vector<Note>> Audio::GetNotesInSimilarOctave(std::vector<Note> notes) {
  // Initialize.
  std::map<unsigned short, std::vector<Note>> notes_in_ocatve;
  for (const auto& note : notes)
    notes_in_ocatve[note.ocatve_].push_back(note);
  // Add notes to ocatve_ above and below.
  for (const auto& it : notes_in_ocatve) {
    if (notes_in_ocatve.count(it.first-1) > 0)
      notes_in_ocatve[it.first-1].insert(notes_in_ocatve[it.first-1].end(), it.second.begin(), it.second.end());
    if (notes_in_ocatve.count(it.first+1) > 0)
      notes_in_ocatve[it.first+1].insert(notes_in_ocatve[it.first+1].end(), it.second.begin(), it.second.end());
  }
  return notes_in_ocatve;
}

std::vector<unsigned short> Audio::GetInterval(std::vector<Note> notes) {
  auto notes_in_ocatve = GetNotesInSimilarOctave(notes);
  std::vector<unsigned short> intervals;
  for (const auto& it : notes_in_ocatve) {
    if (it.second.size() < 2)
      continue;
    for (unsigned int i=1; i<it.second.size(); i++) {
      unsigned short note_a = it.second[i-1].note_;
      unsigned short note_b = it.second[i].note_;
      // Handle different ocatves.
      if (it.second[i-1].ocatve_ > it.second[i].ocatve_)
        note_a += 12;
      else if (it.second[i-1].ocatve_ < it.second[i].ocatve_)
        note_b += 12;
      // Calculate interval
      unsigned short interval = (note_a < note_b) ? note_b - note_a : note_b+12-note_a;
      // Add only if it is between 0 and 12.
      if (interval <= 12)
        intervals.push_back(interval);
    }
  }
  return intervals;
}
