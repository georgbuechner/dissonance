#include "share/objects/dtos.h"
#include <catch2/catch.hpp>
#include <string>

TEST_CASE("test audio transfer", "[transfer]") {
  std::string username = "fux";
  std::string songname = "Union v2.mp3";
  int part = 1;
  int parts = 2;
  std::string content = "hello I am audio content";
  AudioTransferData data((std::string)username, (std::string)songname);
  data.set_parts(parts);
  data.set_part(part);
  data.set_content(content);
  REQUIRE(data.username() == username);
  REQUIRE(data.songname() == songname);
  REQUIRE(data.part() == part);
  REQUIRE(data.parts() == parts);
  REQUIRE(data.content() == content);

  std::string binary_content = data.string();
  AudioTransferData transfered_data(binary_content.c_str(), binary_content.size());
  REQUIRE(data.username() == transfered_data.username());
  REQUIRE(data.songname() == transfered_data.songname());
  REQUIRE(data.part() == transfered_data.part());
  REQUIRE(data.parts() == transfered_data.parts());
  REQUIRE(data.content() == transfered_data.content());
}
