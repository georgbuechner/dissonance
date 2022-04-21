#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <curses.h>
#include <exception>
#include <iostream>
#include <locale>
#include <random>
#include <stdexcept>
#include <vector>

#include "share/defines.h"
#include "spdlog/spdlog.h"

#include "server/game/field/field.h"
#include "share/objects/transfer.h"
#include "share/objects/units.h"
#include "share/constants/codes.h"
#include "server/game/player/player.h"
#include "share/tools/random/random.h"
#include "share/tools/utils/utils.h"

#define SECTIONS 8


Field::Field(int lines, int cols, RandomGenerator* ran_gen) {
  lines_ = lines;
  cols_ = cols;
  ran_gen_ = ran_gen;

  // initialize empty field.
  for (int l=0; l<=lines_; l++) {
    field_.push_back({});
    for (int c=0; c<=cols_; c++)
      field_[l].push_back(SYMBOL_FREE);
  }
}

// getter 
unsigned int Field::lines() { return lines_; }
unsigned int Field::cols() { return cols_; }

std::vector<position_t> Field::GraphPositions() {
  std::vector<position_t> graph_positions;
  for (const auto& it : graph_.nodes())
    graph_positions.push_back(it.second->pos_);
  return graph_positions;
}

std::vector<position_t> Field::AddNucleus(unsigned int num_players) {
  spdlog::get(LOGGER)->info("Field::AddNucleus");
  // Get all availible sections (more than a fourth of the positions of a section exist in graph). 
  std::vector<unsigned int> availible_sections;
  for (unsigned int i=1; i<=8; i++) {
    unsigned int free_positions = 0;
    auto positions_in_section = GetAllPositionsOfSection(i);
    for (const auto& it : positions_in_section) {
      if (graph_.InGraph(it)) 
        free_positions++;
    }
    if (free_positions > positions_in_section.size()/4)
      availible_sections.push_back(i);
  }
  // If not at least half as many sections are availible as players (max 2 players per section) omit.
  if (availible_sections.size() < num_players/2)
    return {};
  // Create nucleus-positions.
  std::vector<position_t> nucleus_positions;
  for (unsigned int i=0; i<num_players; i++) {
    // Get random section and then erase retreived section from availible sections
    int section = ran_gen_->RandomInt(0, availible_sections.size()-1);
    auto positions_in_section = GetAllPositionsOfSection(availible_sections[section], true);
    availible_sections.erase(availible_sections.begin() + section);
    // Make sure no other nucleus is too near.
    position_t pos = positions_in_section[ran_gen_->RandomInt(0, positions_in_section.size()-1)];
    while (NucleusInRange(pos, 8))
      pos = positions_in_section[ran_gen_->RandomInt(0, positions_in_section.size())];
    // Add to field and nucleus positions.
    field_[pos.first][pos.second] = SYMBOL_DEN;
    nucleus_positions.push_back(pos);
    spdlog::get(LOGGER)->info("Adding nucleus at position: {}", utils::PositionToString(pos));
  }
  spdlog::get(LOGGER)->info("Field::AddNucleus: done");
  return nucleus_positions;
}

std::map<int, position_t> Field::AddResources(position_t start_pos) {
  spdlog::get(LOGGER)->info("Field::AddResources {}", utils::PositionToString(start_pos));
  std::map<int, position_t> resource_positions;

  // Get positions sourrounding start position, with enough free spaces for all resources.
  unsigned int nth_try = 0;
  std::vector<position_t> positions = GetAllInRange(start_pos, 4, 2, true);
  spdlog::get(LOGGER)->info("Field::AddResources, positions: {}", positions.size());
  while(positions.size() < symbol_resource_mapping.size()-1) {
    spdlog::get(LOGGER)->debug("Field::AddResources: {}th try getting positions", nth_try);
    positions = GetAllInRange(start_pos, 4+nth_try++, 3, true);
  }
  // Randomly asign each resource one of these free positions.
  for (const auto& it : symbol_resource_mapping) {
    if (it.second == IRON)
      continue;
    spdlog::get(LOGGER)->info("Field::AddResources: resource {}", it.second);
    int ran = ran_gen_->RandomInt(0, positions.size()-1);
    spdlog::get(LOGGER)->info("Field::AddResources: ran {}", ran);
    auto pos = positions[ran];
    positions.erase(positions.begin()+ran);
    field_[pos.first][pos.second] = it.first;
    resource_positions[it.second] = pos;
  }
  spdlog::get(LOGGER)->info("Field::AddResources: done");
  return resource_positions;
}

void Field::BuildGraph() {
  spdlog::get(LOGGER)->info("Field::BuildGraph");
  // Add all nodes.
  spdlog::get(LOGGER)->debug("Field::BuildGraph: Adding nodes...");
  for (int l=0; l<lines_; l++) {
    for (int c=0; c<cols_; c++) {
      if (field_[l][c] != SYMBOL_HILL)
        graph_.AddNode(l, c);
    }
  }
  // For each node, add edges.
  spdlog::get(LOGGER)->debug("Field::BuildGraph: Adding edged...");
  for (auto node : graph_.nodes()) {
    for (const auto& pos : GetAllInRange(node.second->pos_, 1.5, 1)) {
      if (InField(pos) && field_[pos.first][pos.second] != SYMBOL_HILL && graph_.InGraph(pos))
        graph_.AddEdge(node.second, graph_.GetNode(pos));
    }
  }
  // Remove all nodes not in main circle
  spdlog::get(LOGGER)->debug("Field::BuildGraph: reducing to greates component...");
  int num_positions_before = graph_.nodes().size();
  graph_.ReduceToGreatestComponent();
  spdlog::get(LOGGER)->info("Field::BuildGraph: Done. {}/{} positions left!", graph_.nodes().size(), 
      num_positions_before);
}

// void Field::AddHills(RandomGenerator* gen_1, RandomGenerator* gen_2, unsigned short denceness) {
//   spdlog::get(LOGGER)->info("Field::AddHills: denceness={}", denceness);
//   for (int l=0; l<lines_; l++) {
//     for (int c=0; c<cols_; c++) {
//       // Check random 50% chance for building a mountain.
//       if (gen_1->RandomInt(0, 1) == 1) {
//         spdlog::get(LOGGER)->debug("Creating muntain: {}|{}", l, c);
//         field_[l][c] = SYMBOL_HILL;
//         // Get size of mountain (0-5)
//         int level = gen_2->RandomInt(0, 5)-999;
//         if (level < 1)
//           continue;
//         spdlog::get(LOGGER)->debug("Creating {} hills", level);
//         auto positions = GetAllInRange({l, c}, level - denceness, 1);
//         for (const auto& pos : positions)
//           field_[pos.first][pos.second] = SYMBOL_HILL;
//       }
//     }
//   }
//   spdlog::get(LOGGER)->debug("Field::AddHills: done");
// }

void Field::AddHills(RandomGenerator* gen_1, RandomGenerator* gen_2, unsigned short denceness) {
  auto pitches = gen_1->analysed_data().EveryXPitch(lines_*cols_);
  std::reverse(pitches.begin(), pitches.end());
  double avrg = gen_1->analysed_data().average_pitch_;
  spdlog::get(LOGGER)->info("Field::AddHills: num pitches={}, avrg={}, pitch={}", pitches.size(), avrg, pitches.front());
  spdlog::get(LOGGER)->info("Field::AddHills: denceness={} -> avrg/{}", denceness, 1.8+denceness*0.1);
  int mountains = 0;
  int big_mountains = 0;
  
  for (int l=0; l<lines_; l++) {
    for (int c=0; c<cols_; c++) {
      // Check random 50% chance for building a mountain.
      if (pitches.back() < (avrg/(1.8+denceness*0.1)))
        field_[l][c] = SYMBOL_HILL;
      if (pitches.back() < avrg/(3.5+denceness*0.8)) {
        mountains++;
        for (const auto& it : GetAllInRange({l, c}, 1.5, 0))
          field_[it.first][it.second] = SYMBOL_HILL;
      }
      if (pitches.back() < avrg/(4.1+denceness*2)) {
        big_mountains++;
        for (const auto& it : GetAllInRange({l, c}, 2, 1))
          field_[it.first][it.second] = SYMBOL_HILL;
      }

      pitches.pop_back();
    }
  }
  spdlog::get(LOGGER)->debug("Field::AddHills: done, build {} mountains and {} bug mountains", mountains, big_mountains);
}


std::list<position_t> Field::GetWayForSoldier(position_t start_pos, std::vector<position_t> way_points) {
  spdlog::get(LOGGER)->debug("Field::GetWayForSoldier: pos={}", utils::PositionToString(start_pos));
  if (way_points.size() < 1)
    return {};

  std::list<position_t> way = {start_pos};
  for (unsigned int i=0; i<way_points.size(); i++) {
    try {
      // Get new way-part
      auto new_part = graph_.DijkstrasWay(way.back(), way_points[i]);
      new_part.pop_front(); // Remove first element (start-position).
      way.insert(way.end(), new_part.begin(), new_part.end());
      // set start-pos as current way-point
      start_pos = way_points[0];
    }
    catch (std::exception& e) {
      spdlog::get(LOGGER)->error("Field::GetWayForSoldier: Serious error: no way found: {}", e.what());
    }
  }
  return way;
}

void Field::AddNewUnitToPos(position_t pos, int unit) {
  std::unique_lock ul_field(mutex_field_);
  if (unit == UnitsTech::ACTIVATEDNEURON)
    field_[pos.first][pos.second] = SYMBOL_DEF;
  else if (unit == UnitsTech::SYNAPSE)
    field_[pos.first][pos.second] = SYMBOL_BARACK;
  else if (unit == UnitsTech::NUCLEUS)
    field_[pos.first][pos.second] = SYMBOL_DEN;
}

bool Field::InField(position_t pos) {
  return (pos.first >= 0 && pos.first <= lines_ && pos.second >= 0 && pos.second <= cols_);
}

position_t Field::FindFree(position_t pos, int min, int max) {
  std::shared_lock sl_field(mutex_field_);
  auto positions = GetAllInRange(pos, max, min, true);
  if (positions.size() == 0)
    return {-1, -1};
  return positions[ran_gen_->RandomInt(0, positions.size()-1)];
}

std::string Field::GetSymbolAtPos(position_t pos) const {
  return field_[pos.first][pos.second];
}

std::vector<position_t> Field::GetAllInRange(position_t start, double max_dist, double min_dist, bool free) {
  std::vector<position_t> positions_in_range;
  if (!InField(start))
    return positions_in_range;
  position_t upper_corner = {start.first-max_dist, start.second-max_dist};
  for (int i=0; i<=max_dist*2; i++) {
    for (int j=0; j<=max_dist*2; j++) {
      position_t pos = {upper_corner.first+i, upper_corner.second+j};
      if (InField(pos) && utils::InRange(start, pos, min_dist, max_dist)
          && (!free || field_[pos.first][pos.second] == SYMBOL_FREE)
          && (!free || graph_.InGraph(pos)))
        positions_in_range.push_back(pos);
    }
  }
  std::string pos_str = "";
  for (const auto& position : positions_in_range) 
    pos_str += utils::PositionToString(position) + ", ";
  return positions_in_range;
}

std::vector<position_t> Field::GetAllCenterPositionsOfSections() {
  std::vector<position_t> positions;
  for (int i=1; i<=SECTIONS; i++) {
    int l = (i-1)%(SECTIONS/2)*(cols_/4);
    int c = (i < (SECTIONS/2)+1) ? 0 : lines_/2;
    positions.push_back({(c+c+lines_/2)/2, (l+l+cols_/4)/2});
  }
  return positions;
}

std::vector<position_t> Field::GetAllPositionsOfSection(unsigned short interval, bool in_graph) {
  std::vector<position_t> positions;
  int l = (interval-1)%(SECTIONS/2)*(cols_/4);
  int c = (interval < (SECTIONS/2)+1) ? 0 : lines_/2;
  for (int i=l; i<l+cols_/4; i++) {
    for (int j=c; j<c+lines_/2; j++) {
      if (!in_graph || graph_.InGraph({j, i}))
        positions.push_back({j, i});
    }
  }
  return positions;
}

std::vector<std::vector<Transfer::Symbol>> Field::Export(std::vector<Player*> players) {
  std::shared_lock sl_field(mutex_field_);
  std::vector<std::vector<Transfer::Symbol>> t_field;
  // Create transfer-type field.
  for (int l=0; l<lines_; l++) {
    t_field.push_back(std::vector<Transfer::Symbol>());
    for (int c=0; c<cols_; c++) {
      position_t cur = {l, c};
      int color = COLOR_DEFAULT;
      // Check if belongs to either player, is blocked or is resource-neuron
      for (unsigned int i=0; i<players.size(); i++) {
        auto new_color = players[i]->GetColorForPos(cur);
        if (new_color != COLOR_DEFAULT)
          color = new_color;
      }
      // Add to json field.
      t_field[l].push_back({field_[l][c], color});
    }
  }
  return t_field;
}

bool Field::NucleusInRange(position_t pos, unsigned int range) {
  for (const auto& it : GetAllInRange(pos, range, 0))
    if (field_[it.first][it.second] == SYMBOL_DEN)
      return true;
  return false; 
}
