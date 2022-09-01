#ifndef SRC_ANALYSES_H_
#define SRC_ANALYSES_H_

#include "nlohmann/json.hpp"
#include "nlohmann/json_fwd.hpp"
#include <aubio/aubio.h>
#include <aubio/musicutils.h>
#include <aubio/notes/notes.h>
#include <aubio/pitch/pitch.h>
#include <aubio/tempo/tempo.h>
#include <cstddef>
#include <filesystem>
#include <iostream>
#include <list>
#include <map>
#include <queue>
#include <string>
#include <vector>
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

struct Note {
  int midi_note_;
  std::string note_name_;
  int note_;
  int ocatve_;
};

struct AudioDataTimePoint {
  double time_;
  int bpm_;
  int level_;
  std::vector<Note> notes_;
  int interval_;
};

struct Interval {
  int id_;
  std::string key_;
  int key_note_;
  int signature_;  ///< 0=unsigned, 1=sharp, 2=flat
  bool major_;  
  int notes_in_key_;
  int notes_out_key_;
  int darkness_;
};

struct AudioData {
  std::deque<AudioDataTimePoint> data_per_beat_;
  float average_bpm_;
  float average_level_;
  std::vector<double> pitches_;
  double average_pitch_;

  int max_level_;
  std::string key_;
  std::map<int, Interval> intervals_;
  int max_peak_;

  std::vector<double> EveryXPitch(int num_pitches) {
    std::vector<double> pitches;
    int every_x = pitches_.size()/num_pitches;
    for (size_t i=0; i<pitches_.size(); i+=every_x)
      pitches.push_back(pitches_[i]);
    return pitches;
  }

  bool Allegro() { return average_bpm_ >= 120; }
};

class Audio {
  public:
    Audio(std::string base_path);
    Audio(const Audio& audio);
    
    // getter
    AudioData& analysed_data();
    static std::map<std::string, std::vector<std::string>> keys();
    std::string filename(bool shortened) const;
    std::string source_path();

    // setter 
    void set_source_path(std::string source_path);
    
    // methods:
    void Analyze();
    void Analyze(nlohmann::json data);
    nlohmann::json GetAnalyzedData();
    std::vector<double> EveryXPitch(int num_pitches);

    void play();
    
    void Pause();
    void Unpause();
    void Stop();

    bool MoreOffNotes(const AudioDataTimePoint& data_at_beat, bool off=true) const;
    int NextOfNotesIn(double cur_time) const;

    static std::vector<int> GetInterval(std::vector<Note> notes);

    static void Initialize();


  private:
    // members:
    std::string source_path_;
    std::string filename_;
    const std::string base_path_;
    AudioData analysed_data_;
    ma_device device_;
    ma_decoder decoder_;
    static std::map<std::string, std::vector<std::string>> keys_;
    static const std::vector<std::string> note_names_;

    // methods:
    static void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
    static Note ConvertMidiToNote(int midi_note);

    void CreateLevels(int intervals);
    void CalcLevel(int quater, std::map<std::string, int> notes_by_frequency, int darkness);

    void AnalyzePeak();
    AudioData AnalyzeFile(std::string source_path);
    AudioData Load(std::string source_path);
    AudioData Load(nlohmann::json data);
    void Safe(AudioData audio_data, std::string source_path);
    std::string GetOutPath(std::filesystem::path source_path);

    static std::map<int, std::vector<Note>> GetNotesInSimilarOctave(std::vector<Note> notes);

};

#endif
