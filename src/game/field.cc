#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <curses.h>
#include <exception>
#include <iostream>
#include <locale>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "game/field.h"
#include "objects/transfer.h"
#include "objects/units.h"
#include "player/player.h"
#include "random/random.h"
#include "spdlog/spdlog.h"
#include "utils/utils.h"
#include "constants/codes.h"

#define COLOR_DEFAULT 3
#define COLOR_PLAYER 2 
#define COLOR_KI 1 
#define COLOR_RESOURCES 4 
#define COLOR_OK 5 
#define COLOR_HIGHLIGHT 6 

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
int Field::lines() { 
  return lines_; 
}
int Field::cols() { 
  return cols_; 
}

void Field::AddBlink(position_t blink_pos) {
  std::unique_lock ul(mutex_field_);
  blinks_.push_back(blink_pos);
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
  spdlog::get(LOGGER)->debug("Field::AddResources");
  std::map<int, position_t> resource_positions;
  for (const auto& it : resources_symbol_mapping) {
    spdlog::get(LOGGER)->debug("Field::AddResources: resource {} first try getting positions", it.first);
    std::vector<position_t> positions = GetAllInRange(start_pos, 4, 2, true);
    if (positions.size() == 0) {
      spdlog::get(LOGGER)->debug("Field::AddResources: resource {} 2. try getting positions", it.first);
      positions = GetAllInRange(start_pos, 5, 3, true);
    }
    position_t pos = positions[ran_gen_->RandomInt(0, positions.size()-1)];
    spdlog::get(LOGGER)->debug("Field::AddResources: got position {}", utils::PositionToString(pos));
    field_[pos.first][pos.second] = it.first;
    resource_positions[it.second] = pos;
  }
  spdlog::get(LOGGER)->debug("Field::AddResources: done");
  return resource_positions;
}

void Field::BuildGraph(std::vector<position_t> nucleus_positions) {
  // Add all nodes.
  for (int l=0; l<lines_; l++) {
    for (int c=0; c<cols_; c++) {
      if (field_[l][c] != SYMBOL_HILL)
        graph_.AddNode(l, c);
    }
  }

  // For each node, add edges.
  for (auto node : graph_.nodes()) {
    std::vector<position_t> neighbors = GetAllInRange({node.second->line_, node.second->col_}, 1.5, 1);
    for (const auto& pos : neighbors) {
      if (InField(pos) && field_[pos.first][pos.second] != SYMBOL_HILL && graph_.InGraph(pos))
      graph_.AddEdge(node.second, graph_.nodes().at(pos));
    }
  }

  // Remove all nodes not in main circle
  graph_.RemoveInvalid(nucleus_positions.front());
  for (const auto& it : nucleus_positions) 
    if (graph_.nodes().count(it) == 0)
      throw std::logic_error("Invalid world.");
}

void Field::AddHills(RandomGenerator* gen_1, RandomGenerator* gen_2, unsigned short denceness) {
  spdlog::get(LOGGER)->debug("Field::AddHills: denceness={}", denceness);

  for (int l=0; l<lines_; l++) {
    for (int c=0; c<cols_; c++) {
      if (gen_1->RandomInt(0, 1) == 1) {
        spdlog::get(LOGGER)->info("Creating muntain");
        field_[l][c] = SYMBOL_HILL;
        int level = gen_2->RandomInt(0, 5)-999;
        if (level < 1)
          continue;
        spdlog::get(LOGGER)->info("Creating {} hills", level);
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

void Field::UpdateField(Player *player, std::vector<std::vector<std::string>>& field) {
  // Accumulate all ipsps and epsps at their current positions.
  // Accumulate epsps with start symbol '0' and ipsps with start symbol 'a'.
  std::map<position_t, std::map<char, int>> potentials_at_position;
  for (const auto& it : player->potential()) {
    if (it.second.type_ == UnitsTech::EPSP) 
      potentials_at_position[it.second.pos_]['0']++;
    else if (it.second.type_ == UnitsTech::IPSP)
      potentials_at_position[it.second.pos_]['a']++;
  }

  // Add ipsps with increasing letter-count (add ipsps first to prioritise epsps).
  for (const auto& pos : potentials_at_position) {
    int l = pos.first.first;
    int c = pos.first.second;
    // For each type (epsp/ ipsp) create add number of potentials (epsp: 1,2,..;
    // ipsps: a,b,..) to field, if position is free. Add infinity symbol if more
    // than 10 potentials of a kind on one field.
    for (const auto& potential : pos.second) {
      if (field[l][c] == SYMBOL_FREE && potential.second <= 10)
        field[l][c] = potential.first + potential.second;
      else if (field[l][c] == SYMBOL_FREE && potential.second > 10)
        field[l][c] = ":";
    }
  }
}

bool Field::CheckCollidingPotentials(position_t pos, std::vector<Player*> players) {
  bool on_same_field = false;
  for (unsigned int i=0; i<players.size(); i++) {
    for (unsigned int j=0; j<players.size(); j++) {
      if (i==j) 
        continue;
      std::string id_one = players[i]->GetPotentialIdIfPotential(pos);
      std::string id_two = players[j]->GetPotentialIdIfPotential(pos);
      if (id_one.find("epsp") != std::string::npos && id_two.find("ipsp") != std::string::npos) {
        players[i]->NeutralizePotential(id_one, 1);
        players[j]->NeutralizePotential(id_two, -1); // -1 increase potential.
        on_same_field = true;
      }
    }
  }
  return on_same_field;
}

bool Field::InRange(position_t pos, int range, position_t start) {
  if (range == ViewRange::GRAPH)
    return graph_.InGraph(pos);
  return utils::Dist(pos, start) <= range;
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

int Field::GetXInRange(int x, int min, int max) {
  if (x < min) 
    return min;
  if (x > max)
    return max;
  return x;
}

int Field::random_coordinate_shift(int x, int min, int max) {
  // Determine decrease or increase of values.
  int plus_minus = ran_gen_->RandomInt(0, 1);
  int random_faktor = ran_gen_->RandomInt(min, max);
  return x + pow(-1, plus_minus)*random_faktor;
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
  spdlog::get(LOGGER)->info("Field::GetAllInRange: {}", pos_str);
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

std::vector<std::vector<Transfer::Symbol>> Field::ToJson(std::vector<Player*> players) {
  std::shared_lock sl_field(mutex_field_);
  auto field = field_;
  sl_field.unlock();

  for (const auto& it : players)
    UpdateField(it, field);

  std::vector<std::vector<Transfer::Symbol>> t_field;

  for (int l=0; l<lines_; l++) {
    t_field.push_back(std::vector<Transfer::Symbol>());
    for (int c=0; c<cols_; c++) {
      position_t cur = {l, c};
      int color = COLOR_DEFAULT;

      // Check if belongs to either player, is blocked or is resource-neuron
      for (unsigned int i=0; i<players.size(); i++) {
        if (players[i]->GetNeuronTypeAtPosition(cur) != -1 || players[i]->GetPotentialIdIfPotential(cur) != "")
          color = players[i]->color();
        else 
          continue;
        if (players[i]->IsNeuronBlocked(cur) || players[i]->GetNeuronTypeAtPosition(cur) == RESOURCENEURON)
          color = COLOR_RESOURCES;
      }

      if (std::find(blinks_.begin(), blinks_.end(), cur) != blinks_.end())
        color = COLOR_HIGHLIGHT;
      // both players -> cyan
      else if (CheckCollidingPotentials(cur, players))
        color = COLOR_RESOURCES;
      
      // Add to json field.
      t_field[l].push_back({field[l][c], color});
    }
  }
  blinks_.clear();
  return t_field;
}
