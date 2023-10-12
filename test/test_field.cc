#include <catch2/catch.hpp>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <algorithm>
#include <memory>
#include <set>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>
#include "server/game/server_game.h"
#include "share/audio/audio.h"
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
  std::shared_ptr<RandomGenerator> ran_gen = std::make_shared<RandomGenerator>();
  std::shared_ptr<Field> field = std::make_shared<Field>(100, 100, ran_gen);

  SECTION("tests based on graph", "[main, graph]") {
    field->BuildGraph();

    SECTION("test GetWayForSoldier") {
      SECTION("test way-points keep original order") {
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
        auto way = field->GetWay(start_pos, way_points);

        // Way was created with length of min number of way-points and make sure order is correct.
        REQUIRE(way.size() > 4);
        REQUIRE(utils::Index(way, start_pos) == 0);
        REQUIRE(utils::Index(way, start_pos) < utils::Index(way, way_points[0]));
        REQUIRE(utils::Index(way, way_points[0]) < utils::Index(way, way_points[1]));
        REQUIRE(utils::Index(way, way_points[1]) < utils::Index(way, target_pos));
        std::vector<position_t> vec(way.begin(), way.end());
        REQUIRE(vec.size() == way.size());
        for (unsigned int i=0; i<vec.size()-1; i++) {
          REQUIRE(utils::Dist(vec[i], vec[i+1]) < 1.5); // no "jumps" in way.
          REQUIRE(utils::Dist(vec[i], vec[i+1]) >= 1); // no "doubled" elements in way.
        }
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
      for (int l=1; l<field->lines(); l++) {
        for (int c=1; c<field->cols(); c++) {
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
        field->AddNewNeuron(it, std::make_shared<ActivatedNeuron>(it, 0, 0), nullptr);
      auto positions_in_range = field->GetAllInRange({50, 50}, 3, 0, true);
      REQUIRE(positions_in_range.size() == 29-positions_to_block.size());
    }

    SECTION("Variety of ranges") {
      REQUIRE(field->GetAllInRange({50, 50}, 1, 0).size() == 5);
      REQUIRE(field->GetAllInRange({50, 50}, 1, 1).size() == 4);
      REQUIRE(field->GetAllInRange({50, 50}, 1.5, 0).size() == 9);
      REQUIRE(field->GetAllInRange({50, 50}, 1.5, 1).size() == 8);
      REQUIRE(field->GetAllInRange({50, 50}, 1.5, 1.2).size() == 4);
      REQUIRE(field->GetAllInRange({50, 50}, 2, 0).size() == 13);
      REQUIRE(field->GetAllInRange({50, 50}, 2, 1).size() == 12);
      REQUIRE(field->GetAllInRange({50, 50}, 2, 1.2).size() == 8);
      REQUIRE(field->GetAllInRange({50, 50}, 2, 1.8).size() == 4);
    }
  }

  SECTION("test GetAllCenterPositionsOfSections") {
    std::shared_ptr<Field> field = std::make_shared<Field>(50, 100, std::make_shared<RandomGenerator>());
    field->BuildGraph();
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
    std::shared_ptr<Field> field = std::make_shared<Field>(50, 100, std::make_shared<RandomGenerator>());
    field->BuildGraph();
    auto center_positions = field->GetAllCenterPositionsOfSections();
    auto positions = field->GetAllPositionsOfSection(1);
    for (const auto& it : positions)
      REQUIRE(utils::Dist(center_positions[0], it) <= 30);
  }

  SECTION("test AddResources") {
    field->BuildGraph(); // Check free also checks in graph.
    auto nucleus_pos = t_utils::GetRandomPositionInField(field, ran_gen);
    field->AddResources(nucleus_pos);
    auto resource_positions = field->resource_neurons().at(nucleus_pos);
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
      field->AddNewNeuron(pos, std::make_shared<ActivatedNeuron>(pos, 0, 0), nullptr);
      REQUIRE(field->GetSymbolAtPos(pos) == SYMBOL_DEF);
    }
    SECTION("test add synapse to field") {
      field->AddNewNeuron(pos, std::make_shared<Synapse>(pos, 0, 0, DEFAULT_POS, DEFAULT_POS), nullptr);
      REQUIRE(field->GetSymbolAtPos(pos) == SYMBOL_BARACK);
    }
    SECTION("test add nucleus to field") {
      field->AddNewNeuron(pos, std::make_shared<Nucleus>(pos), nullptr);
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
  std::shared_ptr<RandomGenerator> ran_gen = std::make_shared<RandomGenerator>();
  std::shared_ptr<Field> field = std::make_shared<Field>(3, 4, ran_gen);
  field->BuildGraph();

  SECTION("fasted way direction forward") {
    position_t start = {2, 0};
    position_t target = {1, 3};
    auto new_way = field->graph()->DijkstrasWay(start, target);
    REQUIRE(new_way.front() == start);
    REQUIRE(new_way.back() == target);
  }

  SECTION("fasted way direction backward") {
    position_t start = {1, 3};
    position_t target = {2, 0};
    auto new_way = field->graph()->DijkstrasWay(start, target);
    REQUIRE(new_way.front() == start);
    REQUIRE(new_way.back() == target);
  }

  SECTION("way is identical for both directions and for parts") {
    std::shared_ptr<Field> field = std::make_shared<Field>(100, 100, ran_gen);
    spdlog::get(LOGGER)->debug("MARK");
    field->BuildGraph();
    position_t target = {2, 0};
    position_t start = {99, 99};
    auto way_a = field->graph()->DijkstrasWay(start, target);
    auto way_b = field->graph()->DijkstrasWay(target, start);
    int cache_size = field->graph()->GetCacheSize();
    way_b.reverse();
    // Convert to vectors for index-access
    std::vector<position_t> way_a_vec{std::begin(way_a), std::end(way_a)};
    std::vector<position_t> way_b_vec{std::begin(way_b), std::end(way_b)};
    REQUIRE(way_a.size() == way_a_vec.size()); // make sure size has not changed
    REQUIRE(way_b.size() == way_b_vec.size()); // make sure size has not changed
    // Check equality of bath ways (compare size, then elements)
    REQUIRE(way_a.size() == way_b.size()); 
    for (unsigned int i=0; i<way_a_vec.size(); i++)
      REQUIRE(way_a_vec[i] == way_b_vec[i]);
    // Get new way-bs using shifted start
    for (unsigned int shift=1; shift<way_a.size()-1; shift++) {
      way_b = field->graph()->DijkstrasWay(target, way_a_vec[shift]); // search only pary part of way.
      way_b.reverse(); // reverse to check identity
      std::vector<position_t> way_b_vec_2{std::begin(way_b), std::end(way_b)}; // Convert to vector for index-access
      for (unsigned int i=0; i<way_b_vec_2.size(); i++)
        REQUIRE(utils::PositionToString(way_a_vec[i+shift]) == utils::PositionToString(way_b_vec_2[i]));
    }
    REQUIRE(cache_size == field->graph()->GetCacheSize()); // Cache size should have stayed the same.
  }

  SECTION("test speed") {
    auto start_time = std::chrono::steady_clock::now();
    std::shared_ptr<Field> field = std::make_shared<Field>(100, 100, ran_gen);
    field->BuildGraph();
    std::cout << "Time (build field): " << utils::GetElapsed(start_time, std::chrono::steady_clock::now()) << std::endl;
    position_t target = {2, 0};
    position_t start = {99, 99};

    start_time = std::chrono::steady_clock::now();
    auto new_way = field->graph()->DijkstrasWay(start, target);
    std::cout << "Time (DijkstrasWay): " << utils::GetElapsed(start_time, std::chrono::steady_clock::now()) << std::endl;
  }
}

TEST_CASE("test fiboqueue", "[graph]") {
  position_t pos = {5,5};
  FibHeap<double>::FibNode* x = new FibHeap<double>::FibNode(99999, Graph::to_int(pos));
  REQUIRE(Graph::get_pos(x->payload) == pos);
}

TEST_CASE("test priority queue", "[graph]") {
  // Consruct fib-queue
  FibQueue<double> fq;
  for (int i=0; i<10; i++)
    fq.push(99999, Graph::to_int(std::make_pair(i, i)));
  // Decrease priority of random postion to definitly be first postion.
  position_t first_pos = {5,5};
  fq.decrease_key(Graph::to_int(first_pos), 99999, 1.0);
  REQUIRE(Graph::get_pos(fq.pop()) == first_pos);
  // After popping, searching again should fail (no longer the selected postion)
  REQUIRE(Graph::get_pos(fq.pop()) != first_pos);
}

TEST_CASE("test graph-cache", "[graph]") {
  std::shared_ptr<RandomGenerator> ran_gen = std::make_shared<RandomGenerator>();
  std::shared_ptr<Field> field = std::make_shared<Field>(100, 100, ran_gen);
  field->BuildGraph();
  position_t target = {2, 0};
  position_t start = {99, 99};

  auto start_time = std::chrono::steady_clock::now();
  auto way_1 = field->graph()->DijkstrasWay(start, target);
  auto time_1 = utils::GetElapsed(start_time, std::chrono::steady_clock::now());
  start_time = std::chrono::steady_clock::now();
  auto way_2 = field->graph()->DijkstrasWay(start, target);
  auto time_2 = utils::GetElapsed(start_time, std::chrono::steady_clock::now());
  REQUIRE(way_1 == way_2);
  REQUIRE(time_1 > time_2*1000);
}

TEST_CASE("test graph: find way with special conditions", "[graph]") {
  std::shared_ptr<RandomGenerator> ran_gen = std::make_shared<RandomGenerator>();
  std::shared_ptr<Field> field = std::make_shared<Field>(100, 100, ran_gen);
  field->BuildGraph();

  SECTION("Expecting specific length") {
    position_t target = {2, 0};
    position_t start = {2, 2};
    auto way = field->graph()->DijkstrasWay(start, target);
    REQUIRE(way.size() == 3);
  }

  SECTION("Target one field away vertically") {
    position_t target = {2, 0};
    position_t start = {2, 1};
    auto way = field->graph()->DijkstrasWay(start, target);
    REQUIRE(way.size() == 2); // start and target only
  }

  SECTION("Target one field away horizontally") {
    position_t target = {2, 0};
    position_t start = {3, 0};
    auto way = field->graph()->DijkstrasWay(start, target);
    REQUIRE(way.size() == 2); // start and target only
  }

  SECTION("Target one field away diagonally") {
    position_t target = {2, 0};
    position_t start = {3, 1};
    auto way = field->graph()->DijkstrasWay(start, target);
    REQUIRE(way.size() == 2); // start and target only
  }
}


TEST_CASE("test hashing and unhashing positions" "[graph]") {
  for (int16_t i=0;i<1000; i++) {
    for (int16_t j=0;j<1000; j++) {
      position_t pos = {i, j};
      int i_pos = Graph::to_int(pos);
      REQUIRE(Graph::get_pos(i_pos) == pos);
    }
  }
}

TEST_CASE("Test ran gen", "random-generator]") {
  std::shared_ptr<RandomGenerator> ran_gen = std::make_shared<RandomGenerator>();
  int min = 0;
  int max = 10;
  std::set<int> nums;
  for (int i=0; i<10000; i++) {
    auto ran = ran_gen->RandomInt(min, max);
    nums.insert(ran);
    REQUIRE(ran >= min);
    REQUIRE(ran <= max);
  }
  for (int i=min; i<=max; i++) {
    REQUIRE(nums.count(i));
  }
}

TEST_CASE("Test setting up field from server", "[setup_field]") {
  // Create server game
  std::string base_path = "test_data/";
  ServerGame game(35, 76, SINGLE_PLAYER, 2, base_path, nullptr);

  SECTION("Test all examples") {
    std::string source_path = "dissonance/data/examples/";
    for (auto const& dir_entry : std::filesystem::directory_iterator{source_path}) 
      if (dir_entry.path().extension() == ".mp3" || dir_entry.path().extension() == ".wav")
        REQUIRE(game.TestField(dir_entry.path().string()) == true);
  }
  /*
  SECTION("Local show test") {
    std::string source_path = "/media/data/Music/Aurora/All My Demons Greeting Me As A Friend/01 - Runaway.mp3";
    REQUIRE(game.TestField(source_path) == true);
    source_path = "/media/data/Music/Beethoven/Beethoven complete Symphonies/Beethoven_Symphonies_Side11.mp3";
    REQUIRE(game.TestField(source_path) == true);
    source_path = "/media/data/Music/Eminem/Curtain Call The Hits/06 Lose Yourself.mp3";
    REQUIRE(game.TestField(source_path) == true);
    source_path = "/media/data/Music/david music/blank banshee/Blank Banshee - MEGA - 01 BIOS.mp3";
    REQUIRE(game.TestField(source_path) == true);
    source_path = "/media/data/Music/YUNG HURN/1220/03 - Hellwach - prod. Supahoes.ogg";
    REQUIRE(game.TestField(source_path) == true);
  }
  SECTION("Local full test") {
    std::string source_path = "/media/data/Music/Eminem/Curtain Call The Hits/";
    for (auto const& dir_entry : std::filesystem::directory_iterator{source_path}) 
      if (dir_entry.path().extension() == ".mp3" || dir_entry.path().extension() == ".wav")
        REQUIRE(game.TestField(dir_entry.path().string()) == true);
    source_path = "/media/data/Music/Aurora/";
    for (auto const& dir_entry : std::filesystem::directory_iterator{source_path}) 
      if (dir_entry.path().extension() == ".mp3" || dir_entry.path().extension() == ".wav")
        REQUIRE(game.TestField(dir_entry.path().string()) == true);
    source_path = "/media/data/Music/david music/blank banshee/";
    for (auto const& dir_entry : std::filesystem::directory_iterator{source_path}) 
      if (dir_entry.path().extension() == ".mp3" || dir_entry.path().extension() == ".wav")
        REQUIRE(game.TestField(dir_entry.path().string()) == true);
    source_path = "/media/data/Music/YUNG HURN/Love Hotel EP/Yung Hurn - Love Hotel EP (Official Audio) (Full Album).mp3";
    REQUIRE(game.TestField(source_path) == true);
  }
  */
}
