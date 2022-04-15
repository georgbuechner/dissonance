#include <catch2/catch.hpp>
#include <cstddef>
#include <iterator>
#include <algorithm>
#include "share/constants/codes.h"
#include "server/game/field/field.h"
#include "share/defines.h"
#include "share/objects/units.h"
#include "server/game/player/player.h"
#include "share/tools/graph.h"
#include "testing_utils.h"
#include "share/tools/utils/utils.h"
#include "share/tools/random/random.h"
#include "share/tools/fiboheap.h"
#include "share/tools/fiboqueue.h"

TEST_CASE("test_field", "[main]") {
  RandomGenerator* ran_gen = new RandomGenerator();
  Field* field = new Field(100, 100, ran_gen);

  SECTION("tests based on graph", "[main, graph]") {
    field->BuildGraph();

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

TEST_CASE("test graph", "[graph]") {
  RandomGenerator* ran_gen = new RandomGenerator();
  Field* field = new Field(3, 4, ran_gen);
  field->BuildGraph();

  SECTION("fasted way direction forward") {
    position_t start = {2, 0};
    position_t target = {1, 3};
    auto new_way = field->graph().DijkstrasWay(start, target);
    REQUIRE(new_way.front() == start);
    REQUIRE(new_way.back() == target);
  }

  SECTION("fasted way direction backward") {
    position_t start = {1, 3};
    position_t target = {2, 0};
    auto new_way = field->graph().DijkstrasWay(start, target);
    REQUIRE(new_way.front() == start);
    REQUIRE(new_way.back() == target);
  }

  SECTION("test speed") {
    auto start_time = std::chrono::steady_clock::now();
    Field* field = new Field(100, 100, ran_gen);
    field->BuildGraph();
    std::cout << "Time (build field): " << utils::GetElapsed(start_time, std::chrono::steady_clock::now()) << std::endl;
    position_t target = {2, 0};
    position_t start = {99, 99};

    start_time = std::chrono::steady_clock::now();
    auto way = field->graph().FindWay(start, target);
    std::cout << "Time (tiefensuche): " << utils::GetElapsed(start_time, std::chrono::steady_clock::now()) << std::endl;
    start_time = std::chrono::steady_clock::now();
    auto new_way = field->graph().DijkstrasWay(start, target);
    std::cout << "Time (DijkstrasWay): " << utils::GetElapsed(start_time, std::chrono::steady_clock::now()) << std::endl;
    REQUIRE(way.front() == new_way.front());
    REQUIRE(way.back() == new_way.back());
  }
}

TEST_CASE("test fiboqueue", "[graph]") {
  position_t pos = {5,5};
  FibHeap<double>::FibNode* x = new FibHeap<double>::FibNode(99999, Graph::to_int(pos));
  REQUIRE(Graph::get_pos(x->payload) == pos);
}

TEST_CASE("test priority queue", "[graph]") {
  Queue queue;
  FibQueue<double> fq;
  for (int i=0; i<10; i++) {
    position_t pos = {i,i};
    queue.insert(pos, 99999);
    fq.push(99999, Graph::to_int(pos));
  }
  position_t first_pos = {5,5};
  queue.descrease_key(first_pos, 99999, 1);
  fq.decrease_key(Graph::to_int(first_pos), 99999, 1.0);

  REQUIRE(queue.pop_front() == first_pos);
  REQUIRE(Graph::get_pos(fq.pop()) == first_pos);
  // Search again should fail.
  REQUIRE(queue.pop_front() != first_pos);
  REQUIRE(Graph::get_pos(fq.pop()) != first_pos);
}

TEST_CASE("test graph-cache", "[grapj]") {
  RandomGenerator* ran_gen = new RandomGenerator();
  Field* field = new Field(100, 100, ran_gen);
  field->BuildGraph();
  position_t target = {2, 0};
  position_t start = {99, 99};

  auto start_time = std::chrono::steady_clock::now();
  auto way_1 = field->graph().DijkstrasWay(start, target);
  auto time_1 = utils::GetElapsed(start_time, std::chrono::steady_clock::now());
  start_time = std::chrono::steady_clock::now();
  auto way_2 = field->graph().DijkstrasWay(start, target);
  auto time_2 = utils::GetElapsed(start_time, std::chrono::steady_clock::now());
  REQUIRE(way_1 == way_2);
  REQUIRE(time_1 > time_2*1000);
}

TEST_CASE("test hashing and unhashing positions" "[graph]") {
  for (unsigned short i=0;i<1000; i++) {
    for (unsigned short j=0;j<1000; j++) {
      position_t pos = {i, j};
      int i_pos = Graph::to_int(pos);
      REQUIRE(Graph::get_pos(i_pos) == pos);
    }
  }
}

void PrintMiniField(std::vector<std::vector<Transfer::Symbol>> field) {
  for (const auto& i : field) {
    for (const auto& j : i) {
      int color = 30 + j.color_;
      std::cout << "\033[1;" + std::to_string(color) + "m";
      std::cout << j.symbol_ << " ";
      std::cout << "\033[0m";
    }
    std::cout << std::endl;
  }
}


TEST_CASE("create mini field", "[field]") {
  RandomGenerator* ran_gen = new RandomGenerator();
  Field* field = new Field(10, 10, ran_gen);
  field->BuildGraph();
  auto nucleus = field->AddNucleus(1);
  Player* p = new Player(nucleus.front(), field, ran_gen, COLOR_AVAILIBLE);

  auto exported_field = field->Export({p});
  PrintMiniField(exported_field);
}
