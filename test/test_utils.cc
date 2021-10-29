#include <cstddef>
#include <iostream>
#include <catch2/catch.hpp>
#include <map>
#include <list>
#include <vector>
#include "utils/utils.h"

TEST_CASE ("test_dist", "[utils]") {
  REQUIRE(utils::Dist({1,1}, {5,5}) == utils::Dist({5,5}, {1,1}));
  std::cout << "48|48 -> 50|50: " << utils::Dist({48,48}, {50,50});
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

TEST_CASE("test advance.", "") {
  std::list<int> list = {1, 2, 3, 4};
  auto it = list.begin();
  std::advance(it, 6);
  REQUIRE(*it == 2);
}

TEST_CASE("test cycle", "") {

  REQUIRE((((-1)%6)+6)%6 == 5);
  unsigned int cur = 0;
  std::vector<unsigned int> expected = {1,2,3,4,5,0};
  for (unsigned int i=0; i<6; i++) {
    cur = (((cur+1)%6) + 6)%6 ;
    REQUIRE(cur == expected[i]);
  }
  expected = {5, 4, 3, 2, 1, 0};
  for (unsigned int i=0; i<6; i++) {
    int n = cur - 1;
    cur = ((n%6)+6)%6;
    REQUIRE(cur == expected[i]);
  }
}

TEST_CASE("test slicing vector", "") {
  std::vector<int> vec = {1,2,3,4,5,6,7,8,9};
  auto sliced_vec = utils::SliceVector(vec, 1, 3);
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
