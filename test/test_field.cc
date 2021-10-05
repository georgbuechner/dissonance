#include <catch2/catch.hpp>
#include <cstddef>
#include <iterator>
#include <algorithm>
#include "constants/codes.h"
#include "game/field.h"
#include "objects/units.h"
#include "player/player.h"
#include "utils/utils.h"
#include "random/random.h"

TEST_CASE("test_field", "[main]") {
  int lines = 100;
  int cols = 100;
  Field* field = new Field(lines, cols, new RandomGenerator(), new RandomGenerator());

  SECTION("tests based on graph", "[main, graph]") {
    position_t start_pos = field->AddNucleus(8);
    position_t target_pos = field->AddNucleus(1);
    field->BuildGraph(start_pos, target_pos);

    SECTION("test_get_way_for_soldier") {
      SECTION("test way-points are sorted") {
        // Create two way-points, where wp_1 should be following wp_2, so we expect
        // way_point_2 to be passed before wp_1, altough adding wp_1 first.
        position_t way_point_1 = {10, 20};
        position_t way_point_2 = {90, 80};
        // Make sure that we selected wp_1 and wp_2 correctly (wp1 is acctually nearer
        // to target postion.
        REQUIRE(utils::Dist(way_point_1, target_pos) < utils::Dist(way_point_2, target_pos));
        std::vector<position_t> way_points = {way_point_1, way_point_2};

        // Create way
        way_points.push_back(target_pos);
        auto way = field->GetWayForSoldier(start_pos, way_points);

        REQUIRE(way.size() > 4);

        // Get indexes of start, way_points and target
        auto it = std::find(way.begin(), way.end(), start_pos);
        int index_start = std::distance(way.begin(), it);
        it = std::find(way.begin(), way.end(), way_points[0]);
        int index_wp_1 = std::distance(way.begin(), it);
        it = std::find(way.begin(), way.end(), way_points[1]);
        int index_wp_2 = std::distance(way.begin(), it);
        it = std::find(way.begin(), way.end(), target_pos);
        int index_target = std::distance(way.begin(), it);

        // Make sure order is correct.
        REQUIRE(index_start == 0);
        REQUIRE(index_start < index_wp_2);
        REQUIRE(index_wp_2 < index_wp_1);
        REQUIRE(index_wp_1 < index_target);
      }
      SECTION("test way is optimal") {
        std::vector<position_t> way_points = {{10,12}, {63, 64}, {70,74}};
        position_t start_pos = {0, 0};
        auto way = field->GetWayForSoldier(start_pos, way_points);

        size_t expected_length = 0;
        for (const auto& it : way_points) {
          expected_length += field->GetWayForSoldier(start_pos, {it}).size();
          start_pos = it;
        }
        REQUIRE(way.size() == expected_length - (way_points.size()-1));
      }
    }
  }

  SECTION("Test BuildGraph with all positions free.") {
    Graph graph;
    // Add all nodes.
    for (int l=0; l<lines; l++) {
      for (int c=0; c<cols; c++) {
        graph.AddNode(l, c);
      }
    }
    // For each node, add edges.
    for (auto node : graph.nodes()) {
      auto neighbors = field->GetAllInRange({node.second->line_, node.second->col_}, 1.5, 1);
      if (node.second->line_ > 0 && node.second->line_ < lines && node.second->col_ > 0 
          && node.second->col_ < cols)
        REQUIRE(neighbors.size() == 8);
      for (const auto& pos : neighbors) {
        if (graph.InGraph(pos))
          graph.AddEdge(node.second, graph.nodes().at(pos));
      }
    }
    REQUIRE(graph.RemoveInvalid({50, 50}) == 0);
  }

  SECTION("test get positions in range") {
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
      for (int l=1; l<lines; l++) {
        for (int c=1; c<cols; c++) {
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
      field->BuildGraph({0, 0}, {lines-1, cols-1}); // Check free also checks in graph.
      auto positions_to_block = field->GetAllInRange({50, 50}, 2, 0, true);
      for (const auto& it : positions_to_block) 
        field->AddNewUnitToPos(it, UnitsTech::ACTIVATEDNEURON);
      auto positions_in_range = field->GetAllInRange({50, 50}, 3, 0, true);
      REQUIRE(positions_in_range.size() == 29-positions_to_block.size());
    }
  }

  SECTION("test intervals centeres") {
    int lines = 50;
    int cols = 100;
    Field* field = new Field(lines, cols, new RandomGenerator(), new RandomGenerator());
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

  SECTION("test intervals") {
    auto center_positions = field->GetAllCenterPositionsOfSections();
    auto positions = field->GetAllPositionsOfSection(1);
    for (const auto& it : positions) {
      REQUIRE(utils::Dist(center_positions[0], it) <= 30);
    }
  }
}
