#include <iostream>
#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include "utils.h"

TEST_CASE ("test_dist", "[utils]") {
  REQUIRE(utils::dist({1,1}, {5,5}) == utils::dist({5,5}, {1,1}));
}
