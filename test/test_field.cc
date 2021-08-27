#include <catch2/catch.hpp>
#include <iterator>
#include "field.h"
#include "units.h"
#include "utils.h"

TEST_CASE("test_get_way_for_soldier", "[test_field]") {
  int lines = 100;
  int cols = 100;
  Field* field = new Field(100, 100);
  // field->AddHills();
  Position start_pos = field->AddDen(lines/1.5, lines-5, cols/1.5, cols-5);
  Position target_pos = field->AddDen(5, lines/3, 5, cols/3);
  field->BuildGraph(start_pos, target_pos);

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
  REQUIRE(index_start < index_wp_2);
  REQUIRE(index_wp_2 < index_wp_1);
  REQUIRE(index_wp_1 < index_target);
}

