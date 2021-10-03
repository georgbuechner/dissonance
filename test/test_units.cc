#include <iostream>
#include <catch2/catch.hpp>
#include <memory>
#include "objects/units.h"
#include "constants/codes.h"

TEST_CASE("calling getting way has target included", "[units]") {
  std::map<position_t, std::shared_ptr<Neuron>> neurons;
  position_t pos = {1, 1};
  position_t epsp_target = {2,2};
  position_t ipsp_target = {2,2};
  neurons[pos] = std::make_shared<Synapse>(pos, 0, 0, epsp_target, ipsp_target);

  auto way_to_epsp_target = neurons[pos]->GetWayPoints(UnitsTech::EPSP);
  REQUIRE(way_to_epsp_target.size() == 1);
  REQUIRE(way_to_epsp_target.front() == epsp_target);
  auto way_to_ipsp_target = neurons[pos]->GetWayPoints(UnitsTech::IPSP);
  REQUIRE(way_to_ipsp_target.size() == 1);
  REQUIRE(way_to_ipsp_target.front() == ipsp_target);
}
