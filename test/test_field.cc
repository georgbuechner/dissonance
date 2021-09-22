#include <catch2/catch.hpp>
#include <iterator>
#include <algorithm>
#include "constants/codes.h"
#include "game/field.h"
#include "objects/units.h"
#include "utils/utils.h"

TEST_CASE("test_field", "[main]") {
  int lines = 100;
  int cols = 100;
  Field* field = new Field(lines, cols);

  SECTION("tests based on graph", "[main, graph]") {
    Position start_pos = field->AddDen(lines/1.5, lines-5, cols/1.5, cols-5);
    Position target_pos = field->AddDen(5, lines/3, 5, cols/3);
    field->BuildGraph(start_pos, target_pos);

    SECTION("test_get_way_for_soldier") {
      // Create two way-points, where wp_1 should be following wp_2, so we expect
      // way_point_2 to be passed before wp_1, altough adding wp_1 first.
      Position way_point_1 = {10, 20};
      Position way_point_2 = {90, 80};
      // Make sure that we selected wp_1 and wp_2 correctly (wp1 is acctually nearer
      // to target postion.
      REQUIRE(utils::dist(way_point_1, target_pos) < utils::dist(way_point_2, target_pos));
      std::vector<Position> way_points = {way_point_1, way_point_2};

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
      REQUIRE(std::find(positions.begin(), positions.end(), (Position){49, 49}) != positions.end());
      REQUIRE(std::find(positions.begin(), positions.end(), (Position){49, 50}) != positions.end());
      REQUIRE(std::find(positions.begin(), positions.end(), (Position){49, 51}) != positions.end());
      REQUIRE(std::find(positions.begin(), positions.end(), (Position){50, 49}) != positions.end());
      REQUIRE(std::find(positions.begin(), positions.end(), (Position){50, 51}) != positions.end());
      REQUIRE(std::find(positions.begin(), positions.end(), (Position){51, 51}) != positions.end());
      REQUIRE(std::find(positions.begin(), positions.end(), (Position){51, 50}) != positions.end());
      REQUIRE(std::find(positions.begin(), positions.end(), (Position){51, 49}) != positions.end());
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
      auto positions_to_block = field->GetAllInRange({50, 50}, 2, 0, true);
      for (const auto& it : positions_to_block) 
        field->AddNewUnitToPos(it, UnitsTech::ACTIVATEDNEURON);
      auto positions_in_range = field->GetAllInRange({50, 50}, 3, 0, true);
      REQUIRE(positions_in_range.size() == 29-positions_to_block.size());
    }
  }
}
