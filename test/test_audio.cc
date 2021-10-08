#include "catch2/catch.hpp"
#include "audio/audio.h"
#include "constants/codes.h"
#include <algorithm>
#include <vector>

const std::vector<std::string> note_names_ = {
  "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B"
};

Note ConvertMidiToNote(int midi_note) {
  Note note = Note();
  note.midi_note_ = midi_note;
  note.note_ = (midi_note-24)%12;
  note.note_name_ = note_names_[note.note_];
  note.ocatve_ = (midi_note-12)/12;
  return note;
}

TEST_CASE("test createing intervals", "[main]") {
  Audio audio("");
  std::vector<Note> notes = {ConvertMidiToNote(84), ConvertMidiToNote(60), ConvertMidiToNote(87), 
    ConvertMidiToNote(67)};

  auto intervals = audio.GetInterval(notes);

  REQUIRE(intervals.size() == 2);
  REQUIRE(intervals[0] == PERFECT_FIFTH);
  REQUIRE(intervals[1] == MINOR_THIRD);
}

TEST_CASE("test analysing audio", "[main]") {
  // Initialize audio
  Audio::Initialize();

  SECTION("test analysing wav-file") {
    Audio audio("dissonance");
    std::cout << "Created audio object" << std::endl;
    audio.set_source_path("dissonance/data/examples/hellwach.wav");
    std::cout << "Set source path" << std::endl;
    audio.Analyze();
    std::cout << "analysed_data" << std::endl;
    REQUIRE(audio.analysed_data().data_per_beat_.size() > 0);
  }

  // SECTION("test analysing mp3-file") {
  //   Audio audio("dissonance/data/examples/cirrus.mp3");
  //   audio.Analyze();
  //   REQUIRE(audio.analysed_data().data_per_beat_.size() > 0);
  // }
}
