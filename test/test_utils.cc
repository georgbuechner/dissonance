#include <iostream>
#include <catch2/catch.hpp>
#include <map>
#include "utils.h"

TEST_CASE ("test_dist", "[utils]") {
  REQUIRE(utils::dist({1,1}, {5,5}) == utils::dist({5,5}, {1,1}));
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
