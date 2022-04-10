#include <catch2/catch.hpp>
#include <cstddef>
#include <iterator>
#include <algorithm>
#include "share/constants/codes.h"
#include "server/game/field/field.h"
#include "share/objects/units.h"
#include "server/game/player/player.h"
#include "testing_utils.h"
#include "share/tools/utils/utils.h"
#include "share/tools/random/random.h"

TEST_CASE("test_field", "[main]") {
  RandomGenerator* ran_gen = new RandomGenerator();
  Field* field = new Field(100, 100, ran_gen);

  SECTION("tests based on graph", "[main, graph]") {
    field->BuildGraph();
    // auto nucleus_positions = field->AddNucleus(2); // create two nucleus.
    //position_t start_pos = nucleus_positions[0];
    //position_t target_pos = nucleus_positions[1];

    SECTION("test GetWayForSoldier") {
      SECTION("test way-points are sorted") {
        // Create two way-points, where wp_1 should be following wp_2, so we expect
        // way_point_2 to be passed before wp_1, altough adding wp_1 first.
        position_t start_pos = {99, 99};
        position_t target_pos = {1, 1};

        position_t way_point_1 = {10, 20};
        position_t way_point_2 = {90, 80};
        // Make sure that we selected wp_1 and wp_2 correctly (wp1 is acctually nearer
        // to target postion).
        REQUIRE(utils::Dist(way_point_1, target_pos) < utils::Dist(way_point_2, target_pos));
        std::vector<position_t> way_points = {way_point_1, way_point_2, target_pos};

        // Create way
        auto way = field->GetWayForSoldier(start_pos, way_points);

        // Way was created with length of min number of way-points and make sure order is correct.
        REQUIRE(way.size() > 4);
        REQUIRE(utils::Index(way, start_pos) == 0);
        REQUIRE(utils::Index(way, start_pos) < utils::Index(way, way_points[1]));
        REQUIRE(utils::Index(way, way_points[1]) < utils::Index(way, way_points[0]));
        REQUIRE(utils::Index(way, start_pos) < utils::Index(way, target_pos));
      }
    }
  }

  SECTION("test GetAllInRange") {
    SECTION("Don't check free, range=1") {
      auto positions = field->GetAllInRange({50, 50}, 1, 1);
      REQUIRE(positions.size() == 4);
    }

    SECTION("Don't check free, range=1.5") {
      auto positions = field->GetAllInRange({50, 50}, 1.5, 1);
      REQUIRE(positions.size() == 8);
      REQUIRE(std::find(positions.begin(), positions.end(), (position_t){49, 49}) != positions.end());
      REQUIRE(std::find(positions.begin(), positions.end(), (position_t){49, 50}) != positions.end());
      REQUIRE(std::find(positions.begin(), positions.end(), (position_t){49, 51}) != positions.end());
      REQUIRE(std::find(positions.begin(), positions.end(), (position_t){50, 49}) != positions.end());
      REQUIRE(std::find(positions.begin(), positions.end(), (position_t){50, 51}) != positions.end());
      REQUIRE(std::find(positions.begin(), positions.end(), (position_t){51, 51}) != positions.end());
      REQUIRE(std::find(positions.begin(), positions.end(), (position_t){51, 50}) != positions.end());
      REQUIRE(std::find(positions.begin(), positions.end(), (position_t){51, 49}) != positions.end());
    }

    SECTION("Test for each position.") {
      for (unsigned int l=1; l<field->lines(); l++) {
        for (unsigned int c=1; c<field->cols(); c++) {
          auto positions = field->GetAllInRange({l, c}, 1.5, 1);
          if (l == 0 && c == 0)
            REQUIRE(positions.size() == 3);
          else if (l == 0 || c == 0)
            REQUIRE(positions.size() == 5);
          else 
            REQUIRE(positions.size() == 8);
        }
      }
    }

    SECTION("Don't check free, range=2") {
      auto positions_in_range = field->GetAllInRange({30, 50}, 2, 1);
      REQUIRE(positions_in_range.size() == 12);
    }

    SECTION("Check free, range=3") {
      field->BuildGraph(); // Check free also checks in graph.
      auto positions_to_block = field->GetAllInRange({50, 50}, 2, 0, true);
      for (const auto& it : positions_to_block) 
        field->AddNewUnitToPos(it, UnitsTech::ACTIVATEDNEURON);
      auto positions_in_range = field->GetAllInRange({50, 50}, 3, 0, true);
      REQUIRE(positions_in_range.size() == 29-positions_to_block.size());
    }
  }

  SECTION("test GetAllCenterPositionsOfSections") {
    Field* field = new Field(50, 100, new RandomGenerator());
    auto positions= field->GetAllCenterPositionsOfSections();
    REQUIRE(positions.size() == 8);
    REQUIRE(std::find(positions.begin(), positions.end(), (position_t){12,12}) != positions.end());
    REQUIRE(std::find(positions.begin(), positions.end(), (position_t){12,37}) != positions.end());
    REQUIRE(std::find(positions.begin(), positions.end(), (position_t){12,62}) != positions.end());
    REQUIRE(std::find(positions.begin(), positions.end(), (position_t){12,87}) != positions.end());
    REQUIRE(std::find(positions.begin(), positions.end(), (position_t){37,12}) != positions.end());
    REQUIRE(std::find(positions.begin(), positions.end(), (position_t){37,37}) != positions.end());
    REQUIRE(std::find(positions.begin(), positions.end(), (position_t){37,62}) != positions.end());
    REQUIRE(std::find(positions.begin(), positions.end(), (position_t){37,87}) != positions.end());
  }

  SECTION("test GetAllPositionsOfSection") {
    auto center_positions = field->GetAllCenterPositionsOfSections();
    auto positions = field->GetAllPositionsOfSection(1);
    for (const auto& it : positions)
      REQUIRE(utils::Dist(center_positions[0], it) <= 30);
  }

  SECTION("test AddResources") {
    field->BuildGraph(); // Check free also checks in graph.
    auto resource_positions = field->AddResources(t_utils::GetRandomPositionInField(field, ran_gen));
    // One resource was create for each existing resource in resource-mapping (all resources expect iron)
    REQUIRE(resource_positions.size() == resource_symbol_mapping.size()-1); 
    // no two resources have an idenical position:
    std::set<position_t> positions;
    for (const auto& it : resource_positions) 
      positions.insert(it.second);
    REQUIRE(positions.size() == resource_positions.size());
  }

  SECTION ("test AddNewUnitToPos") {
    auto pos = t_utils::GetRandomPositionInField(field, ran_gen);
    SECTION("test add activated neuron to field") {
      field->AddNewUnitToPos(pos, UnitsTech::ACTIVATEDNEURON);
      REQUIRE(field->GetSymbolAtPos(pos) == SYMBOL_DEF);
    }
    SECTION("test add synapse to field") {
      field->AddNewUnitToPos(pos, UnitsTech::SYNAPSE);
      REQUIRE(field->GetSymbolAtPos(pos) == SYMBOL_BARACK);
    }
    SECTION("test add nucleus to field") {
      field->AddNewUnitToPos(pos, UnitsTech::NUCLEUS);
      REQUIRE(field->GetSymbolAtPos(pos) == SYMBOL_DEN);
    }
  }

  SECTION ("test FindFree") {
    // Set up graph. 
    field->BuildGraph(); // Check free also checks in graph.

    SECTION("valid position") {
      position_t start_pos = t_utils::GetRandomPositionInField(field, ran_gen);
      auto pos = field->FindFree(start_pos, 1, 2);
      REQUIRE(field->GetSymbolAtPos(pos) == SYMBOL_FREE);
      REQUIRE(utils::Dist(start_pos, pos) <= 2);
      REQUIRE(utils::Dist(start_pos, pos) >= 1);
    }

    SECTION ("Position outside of field") {
      position_t expected_pos = {-1, -1};
      REQUIRE(field->FindFree({-1, -1}, 1, 2) == expected_pos);
      REQUIRE(field->FindFree({field->lines()+1, field->cols()}, 1, 2) == expected_pos);
    }
  }
}
