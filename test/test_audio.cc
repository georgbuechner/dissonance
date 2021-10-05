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
