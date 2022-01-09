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

position_t Field::AddNucleus(int section) {
  spdlog::get(LOGGER)->debug("Field::AddNucleus");
  auto positions_in_section = GetAllPositionsOfSection(section);
  position_t pos = positions_in_section[ran_gen_->RandomInt(0, positions_in_section.size())];
  field_[pos.first][pos.second] = SYMBOL_DEN;
  // Mark positions surrounding nucleus as free:
  for (const auto& it : GetAllInRange(pos, 1.5, 1))
    field_[it.first][it.second] = SYMBOL_FREE;
  spdlog::get(LOGGER)->debug("Field::AddNucleus: done");
  return pos;
}

std::map<int, position_t> Field::AddResources(position_t start_pos) {
  spdlog::get(LOGGER)->info("Field::AddResources");
  std::map<int, position_t> resource_positions;

  // Get positions sourrounding start position, with enough free spaces for all resources.
  unsigned int nth_try = 0;
  std::vector<position_t> positions = GetAllInRange(start_pos, 4, 2, true);
  while(positions.size() < resources_symbol_mapping.size()) {
    spdlog::get(LOGGER)->debug("Field::AddResources: {}th try getting positions", nth_try);
    positions = GetAllInRange(start_pos, 4+nth_try++, 3, true);
  }

  // Randomly asign each resource one of these free positions.
  for (const auto& it : resources_symbol_mapping) {
    int ran = ran_gen_->RandomInt(0, positions.size());
    auto pos = positions[ran];
    positions.erase(positions.begin()+ran);
    field_[pos.first][pos.second] = it.first;
    resource_positions[it.second] = pos;
  }
  spdlog::get(LOGGER)->debug("Field::AddResources: done");
  return resource_positions;
}

void Field::BuildGraph(std::vector<position_t> nucleus_positions) {
  spdlog::get(LOGGER)->info("Field::BuildGraph: for {} nucleus positions", nucleus_positions.size());

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
    for (const auto& pos : GetAllInRange({node.second->line_, node.second->col_}, 1.5, 1)) {
      if (InField(pos) && field_[pos.first][pos.second] != SYMBOL_HILL && graph_.InGraph(pos))
        graph_.AddEdge(node.second, graph_.nodes().at(pos));
    }
  }

  // Remove all nodes not in main circle
  spdlog::get(LOGGER)->debug("Field::BuildGraph: checking whether all nucleus can be reached...");
  for (const auto& it : nucleus_positions) {
    if (graph_.nodes().count(it) > 0)
      graph_.RemoveInvalid(it);
  }
  for (const auto& it : nucleus_positions) 
    if (graph_.nodes().count(it) == 0)
      throw std::logic_error("Invalid world.");
  spdlog::get(LOGGER)->info("Field::BuildGraph: Done!");
}

void Field::AddHills(RandomGenerator* gen_1, RandomGenerator* gen_2, unsigned short denceness) {
  spdlog::get(LOGGER)->info("Field::AddHills: denceness={}", denceness);
  for (int l=0; l<lines_; l++) {
    for (int c=0; c<cols_; c++) {
      // Check random 50% chance for building a mountain.
      if (gen_1->RandomInt(0, 1) == 1) {
        spdlog::get(LOGGER)->debug("Creating muntain: {}|{}", l, c);
        field_[l][c] = SYMBOL_HILL;
        // Get size of mountain (0-5)
        int level = gen_2->RandomInt(0, 5)-999;
        if (level < 1)
          continue;
        spdlog::get(LOGGER)->debug("Creating {} hills", level);
        auto positions = GetAllInRange({l, c}, level - denceness, 1);
        for (const auto& pos : positions)
          field_[pos.first][pos.second] = SYMBOL_HILL;
      }
    }
  }
  spdlog::get(LOGGER)->debug("Field::AddHills: done");
}

std::list<position_t> Field::GetWayForSoldier(position_t start_pos, std::vector<position_t> way_points) {
  spdlog::get(LOGGER)->info("Field::GetWayForSoldier: pos={}", utils::PositionToString(start_pos));
  position_t target_pos = way_points.back();
  way_points.pop_back();
  std::list<position_t> way = {start_pos};
  // If there are way_points left, sort way-points by distance, then create way.
  if (way_points.size() > 0) {
    std::map<int, position_t> sorted_way;
    for (const auto& it : way_points) 
      sorted_way[lines_+cols_-utils::Dist(it, target_pos)] = it;
    for (const auto& it : sorted_way) {
      try {
        auto new_part = graph_.find_way(way.back(), it.second);
        way.pop_back();
        way.insert(way.end(), new_part.begin(), new_part.end());
      }
      catch (std::exception& e) {
        spdlog::get(LOGGER)->error("Field::GetWayForSoldier: Serious error: no way found: {}", e.what());
      }
    }
  }
  // Create way from last position to target.
  try {
    auto new_part = graph_.find_way(way.back(), target_pos);
    way.pop_back();
    way.insert(way.end(), new_part.begin(), new_part.end());
  }
  catch (std::exception& e) {
    spdlog::get(LOGGER)->error("Field::GetWayForSoldier: Serious error: no way found: {}", e.what());
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

std::vector<position_t> Field::GetAllPositionsOfSection(unsigned short interval) {
  std::vector<position_t> positions;
  int l = (interval-1)%(SECTIONS/2)*(cols_/4);
  int c = (interval < (SECTIONS/2)+1) ? 0 : lines_/2;
  for (int i=l; i<l+cols_/4; i++) {
    for (int j=c; j<c+lines_/2; j++)
      positions.push_back({j, i});
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
        if (players[i]->GetNeuronTypeAtPosition(cur) == RESOURCENEURON)
          color = COLOR_RESOURCES;
        else if (players[i]->GetNeuronTypeAtPosition(cur) != -1)
          color = players[i]->color();
      }
      // Add to json field.
      t_field[l].push_back({field_[l][c], color});
    }
  }
  return t_field;
}
