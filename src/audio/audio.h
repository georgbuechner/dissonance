#ifndef SRC_ANALYSES_H_
#define SRC_ANALYSES_H_

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
#include <vector>
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

struct Note {
  size_t midi_note_;
  std::string note_name_;
  size_t note_;
  size_t ocatve_;
};

struct AudioDataTimePoint {
  double time_;
  int bpm_;
  int level_;
  std::vector<Note> notes_;
  int interval_;
};

struct Interval {
  size_t id_;
  std::string key_;
  size_t key_note_;
  size_t signature_;  ///< 0=unsigned, 1=sharp, 2=flat
  bool major_;  ///< 0=unsigned, 1=sharp, 2=flat
  size_t notes_in_key_;
  size_t notes_out_key_;
  size_t darkness_;
};

struct AudioData {
  std::list<AudioDataTimePoint> data_per_beat_;
  float average_bpm_;
  float average_level_;
  std::string key_;
  std::map<int, Interval> intervals_;
};

class Audio {
  public:
    Audio(std::string base_path);
    
    // getter
    AudioData& analysed_data();
    static std::map<std::string, std::vector<std::string>> keys();

    
    // setter 
    void set_source_path(std::string source_path);
    
    // methods:
    void Analyze();
    void play();
    
    void Pause();
    void Unpause();
    void Stop();

    bool MoreOffNotes(const AudioDataTimePoint& data_at_beat, bool off=true) const;
    size_t NextOfNotesIn(double cur_time) const;

    static void Initialize();

    int RandomInt(size_t min, size_t max);


  private:
    // members:
    std::string source_path_;
    const std::string base_path_;
    AudioData analysed_data_;
    ma_device device_;
    ma_decoder decoder_;
    static std::map<std::string, std::vector<std::string>> keys_;
    static const std::vector<std::string> note_names_;
    size_t last_point_;

    // methods:
    static void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
    static Note ConvertMidiToNote(int midi_note);

    void CreateLevels(int intervals);
    void CalcLevel(size_t quater, std::map<std::string, int> notes_by_frequency, size_t darkness);

    AudioData AnalyzeFile(std::string source_path);
    AudioData Load(std::string source_path);
    void Safe(AudioData audio_data, std::string source_path);
    std::string GetOutPath(std::filesystem::path source_path);
};

#endif
