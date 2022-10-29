#include <filesystem>
#include <catch2/catch.hpp>
#include <map>
#include <list>
#include <vector>
#include "share/defines.h"
#include "share/tools/audio_receiver.h"
#include "share/tools/utils/utils.h"

TEST_CASE ("test_dist", "[utils]") {
  REQUIRE(utils::Dist({1,1}, {5,5}) == utils::Dist({5,5}, {1,1}));
  REQUIRE(utils::InRange({48,48}, {50,50}, 1, 2) == false);  // sqrt(8) > 2
}

TEST_CASE("loops", "[utils]") {
  const std::vector<int> vec_a = {2};
  const std::vector<int> vec_b = {};
  int c=0;
  for (size_t i=0; i<vec_a.size() && i<vec_b.size(); i++)
    c++;
  REQUIRE(c==0);
}

TEST_CASE("bitwise", "[utils]") {
  int num = 0;
  num |= 1UL << 0;
  REQUIRE(num == 1);
  num |= 1UL << 1;
  REQUIRE(num == 3);
  num = 0;
  REQUIRE(num == 0);
}

TEST_CASE("test_map", "[general]") {
  std::map<std::string, int> my_map;
  my_map["test"]++;
  my_map["test"]++;
  my_map["test"]++;
  my_map["test_2"]++;
  REQUIRE(my_map["test"] == 3);
  REQUIRE(my_map["test_2"] == 1);
}

TEST_CASE("test advance.", "") {
  const std::list<int> list = {1, 2, 3, 4};
  auto it = list.begin();
  std::advance(it, 6);
  REQUIRE(*it == 2);
}

TEST_CASE("test cycle", "") {

  REQUIRE((((-1)%6)+6)%6 == 5);
  int cur = 0;
  std::vector<int> expected = {1,2,3,4,5,0};
  for (int i=0; i<6; i++) {
    cur = (((cur+1)%6) + 6)%6 ;
    REQUIRE(cur == expected[i]);
  }
  expected = {5, 4, 3, 2, 1, 0};
  for (int i=0; i<6; i++) {
    int n = cur - 1;
    cur = ((n%6)+6)%6;
    REQUIRE(cur == expected[i]);
  }
}

TEST_CASE("test slicing vector", "") {
  const std::vector<int> vec = {1,2,3,4,5,6,7,8,9};
  const auto sliced_vec = utils::SliceVector(vec, 1, 3);
  REQUIRE(sliced_vec.size() == 3);
  REQUIRE(sliced_vec[0] == 2);
  REQUIRE(sliced_vec[1] == 3);
  REQUIRE(sliced_vec[2] == 4);
}

TEST_CASE("erase from vector", "") {
  std::vector<int> vec = {1,2,3,4,5,6,7,8,9};
  size_t size = vec.size();
  vec.erase(vec.begin());
  REQUIRE(vec.size() == size-1);
  REQUIRE(vec.front() == 2);
}

TEST_CASE("duration") {
  auto start = std::chrono::steady_clock::now();
  // Do some random operation.
  std::string content = utils::GetMedia("dissonance/data/examples/elle_rond_elle_bon_et_blonde.wav");
  content.clear();
  auto end = std::chrono::steady_clock::now();
  REQUIRE(utils::GetElapsedNano(start, end) > 0);
  REQUIRE(utils::GetElapsedNano(end, start) < 0);
}

TEST_CASE("audio-receiver", "[utils]") {
  const std::string username = "analysis";
  const std::string filename = "elle_rond_elle_bon_et_blonde.wav";

  std::string content = utils::GetMedia("dissonance/data/examples/" + filename);
  AudioReceiver audio_receiver(TEST_DATA_PATH);
  audio_receiver.set_audio_data(content);
  std::shared_ptr<Data> audio_data = std::make_shared<AudioTransferData>(username, filename);

  // Test chunking and then rebuilding content.
  auto chunks = audio_receiver.GetCunks();
  std::string rebuilt_content = "";
  for (const auto& it : chunks) {
    rebuilt_content += it.second;
  }
  REQUIRE(content == rebuilt_content );

  // Test "sending" audio-data
  REQUIRE(audio_receiver.audio_stored() == false);
  audio_data->set_parts(chunks.size());
  audio_receiver.Clear();
  int audio_stored_counter = 0;
  for (const auto& it : chunks) {
    audio_data->set_part(it.first);
    audio_data->set_content(it.second);
    audio_receiver.AddData(audio_data);
    if (audio_receiver.audio_stored())
      audio_stored_counter++;
  }
  // Make sure audio is stored and only on last chunk audio was stored.
  REQUIRE(audio_receiver.audio_stored() == true);
  REQUIRE(audio_stored_counter == 1);
  // Make sure new chunks are identical to old
  auto new_chunks = audio_receiver.GetCunks();
  REQUIRE(chunks.size() == new_chunks.size());
  for (const auto& it : chunks) {
    REQUIRE(new_chunks.at(it.first) == it.second);
  }
  std::filesystem::remove_all(TEST_DATA_PATH + username + "/" + filename);
}

TEST_CASE("Test min-max-avrg", "[utils]") {
  const nlohmann::json analysis_json = utils::LoadJsonFromDisc(TEST_DATA_PATH "analysis/elle_rond.json");
  std::vector<double> pitches = analysis_json["pitches"].get<std::vector<double>>();
  std::vector<int> levels;
  for (const auto& it : analysis_json["time_points"].get<nlohmann::json>())
    levels.push_back(it["level"]);

  SECTION ("test min-max-avrg with vector of doubles") {
    double max = -1;
    double min = 10000;
    double avrg = 0;
    for (const auto& it : pitches) {
      if (it > max)
        max = it;
      if (it < min)
        min = it;
      avrg+=it;
    }
    avrg /= pitches.size();
    auto min_max_avrg = utils::GetMinMaxAvrg(pitches);
    REQUIRE(min == min_max_avrg._min);
    REQUIRE(max == min_max_avrg._max);
    REQUIRE(avrg == min_max_avrg._avrg);
  }

  SECTION ("test min-max-avrg with vector of doubles") {
    int max = -1;
    int min = 10000;
    double avrg = 0;
    for (const auto& it : levels) {
      if (it > max)
        max = it;
      if (it < min)
        min = it;
      avrg+=it;
    }
    avrg /= levels.size();
    auto min_max_avrg = utils::GetMinMaxAvrg(levels);
    REQUIRE(min == min_max_avrg._min);
    REQUIRE(max == min_max_avrg._max);
    REQUIRE(avrg == min_max_avrg._avrg);
  }
}

TEST_CASE("Test getting percent", "[utils]") {
  REQUIRE(utils::GetPercentDiff(10, 7) == 30);
  REQUIRE(utils::GetPercentDiff(7, 10) > utils::GetPercentDiff(10, 7));
}

TEST_CASE("Test DouglasPeucker algorithm", "[douglas_peucker]") {
  const nlohmann::json analysis_json = utils::LoadJsonFromDisc(TEST_DATA_PATH "analysis/elle_rond.json");
  const std::vector<double> pitches = analysis_json["pitches"].get<std::vector<double>>();
  std::vector<int> levels;
  for (const auto& it : analysis_json["time_points"].get<nlohmann::json>())
    levels.push_back(it["level"]);

  SECTION("Test with float vector") {
    // Analyze some data for later comparison
    auto min_max_avrg = utils::GetMinMaxAvrg(pitches);
    // Generate some example max-elems
    std::vector<int> test_maxs = {static_cast<int>((int)pitches.size()*0.9), 
      static_cast<int>((int)pitches.size()*0.75), static_cast<int>((int)pitches.size()*0.5), 1000, 500};
    // For each max-elems bound decimate curve.
    for (const auto& max_elems : test_maxs) {
      auto reduced_pitches = utils::DecimateCurveReconverted(pitches, max_elems);
      // Check that new size is in bounds of 10% to given allowed max.
      REQUIRE(utils::GetPercentDiff(reduced_pitches.size(), max_elems) < 7);
      // Check that min, max and average have remained identical
      auto new_min_max_avrg = utils::GetMinMaxAvrg(reduced_pitches);
      REQUIRE(new_min_max_avrg._min == min_max_avrg._min);
      REQUIRE(new_min_max_avrg._max == min_max_avrg._max);
    }
  }

  SECTION("Test with int vector") {
    // Analyze some data for later comparison
    const auto min_max_avrg = utils::GetMinMaxAvrg(levels);
    // Generate some example max-elems
    const std::vector<int> test_maxs = {static_cast<int>((int)levels.size()*0.9), 
      static_cast<int>((int)levels.size()*0.75), static_cast<int>((int)levels.size()*0.5), 100, 75};
    // For each max-elems bound decimate curve.
    for (const auto& max_elems : test_maxs) {
      auto reduced_levels = utils::DecimateCurveReconverted(levels, max_elems);
      // Check that new size is in bounds of 10% to given allowed max.
      REQUIRE(utils::GetPercentDiff(reduced_levels.size(), max_elems) < 7);
      // Check that min, max and average have remained identical
      auto new_min_max_avrg = utils::GetMinMaxAvrg(levels);
      REQUIRE(new_min_max_avrg._min == min_max_avrg._min);
      REQUIRE(new_min_max_avrg._max == min_max_avrg._max);
    }
  }
}
