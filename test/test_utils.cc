#include <iostream>
#include <catch2/catch.hpp>
#include <map>
#include "utils/utils.h"

TEST_CASE ("test_dist", "[utils]") {
  REQUIRE(utils::dist({1,1}, {5,5}) == utils::dist({5,5}, {1,1}));
  std::cout << "48|48 -> 50|50: " << utils::dist({48,48}, {50,50});
  REQUIRE(utils::InRange({48,48}, {50,50}, 1, 2) == false);  // sqrt(8) > 2
}

TEST_CASE("loops", "[utils]") {
  std::vector<int> vec_a = {2};
  std::vector<int> vec_b = {};
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
