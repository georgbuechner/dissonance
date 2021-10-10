#include <bits/types/FILE.h>
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


Field::Field(int lines, int cols, RandomGenerator* ran_gen, int left_border) {
  lines_ = lines;
  cols_ = cols;
  ran_gen_ = ran_gen;
  left_border_ = left_border;

  // initialize empty field.
  for (int l=0; l<=lines_; l++) {
    field_.push_back({});
    for (int c=0; c<=cols_; c++)
      field_[l].push_back(SYMBOL_FREE);
  }

  highlight_ = {};
  range_ = ViewRange::HIDE;
}

// getter
int Field::lines() { 
  return lines_; 
}
int Field::cols() { 
  return cols_; 
}
std::vector<position_t> Field::highlight() {
  return highlight_;
}

// setter:
void Field::set_highlight(std::vector<position_t> positions) {
  highlight_ = positions;
}
void Field::set_range(int range) {
  range_ = range;
}
void Field::set_range_center(position_t pos) {
  range_center_ = pos;
}
void Field::set_replace(std::map<position_t, char> replacements) {
  replacements_ = replacements;
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
  auto positions_arround_nucleus = GetAllInRange({pos.first, pos.second}, 1.5, 1);
  for (const auto& it : positions_arround_nucleus)
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

void Field::BuildGraph(position_t player_den, position_t enemy_den) {
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
      if (InField(pos.first, pos.second) && field_[pos.first][pos.second] != SYMBOL_HILL 
          && graph_.InGraph(pos))
      graph_.AddEdge(node.second, graph_.nodes().at(pos));
    }
  }

  // Remove all nodes not in main circle
  graph_.RemoveInvalid(player_den);
  if (graph_.nodes().count(enemy_den) == 0)
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

position_t Field::GetNewSoldierPos(position_t pos) {
  auto new_pos = FindFree(pos.first, pos.second, 1, 3);
  while(!graph_.InGraph(new_pos))
    new_pos = FindFree(pos.first, pos.second, 1, 3);
  return new_pos;
}

std::list<position_t> Field::GetWayForSoldier(position_t start_pos, std::vector<position_t> way_points) {
  position_t target_pos = way_points.back();
  way_points.pop_back();
  std::list<position_t> way = {start_pos};
  // If there are way_points left, sort way-points by distance, then create way.
  if (way_points.size() > 0) {
    std::map<int, position_t> sorted_way;
    for (const auto& it : way_points) 
      sorted_way[lines_+cols_-utils::Dist(it, target_pos)] = it;
    for (const auto& it : sorted_way) {
      auto new_part = graph_.find_way(way.back(), it.second);
      way.pop_back();
      way.insert(way.end(), new_part.begin(), new_part.end());
    }
  }
  // Create way from last position to target.
  auto new_part = graph_.find_way(way.back(), target_pos);
  way.pop_back();
  way.insert(way.end(), new_part.begin(), new_part.end());
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

bool Field::CheckCollidingPotentials(position_t pos, Player* player_one, Player* player_two) {
  std::string id_one = player_one->GetPotentialIdIfPotential(pos);
  std::string id_two = player_two->GetPotentialIdIfPotential(pos);
  // Not colliding potentials as at at least one position there is no potential.
  if (id_one == "" || id_two == "")
    return false;

  if (id_one.find("epsp") != std::string::npos && id_two.find("ipsp") != std::string::npos) {
    spdlog::get(LOGGER)->debug("Field::CheckCollidingPotentials: calling neutralize potential 1");
    player_one->NeutralizePotential(id_one, 1);
    spdlog::get(LOGGER)->debug("Field::CheckCollidingPotentials: calling neutralize potential 2");
    player_two->NeutralizePotential(id_two, -1); // -1 increase potential.
  }
  else if (id_one.find("ipsp") != std::string::npos && id_two.find("epsp") != std::string::npos) {
    spdlog::get(LOGGER)->debug("Field::CheckCollidingPotentials: calling neutralize potential 1");
    player_one->NeutralizePotential(id_one, -1);
    spdlog::get(LOGGER)->debug("Field::CheckCollidingPotentials: calling neutralize potential 2");
    player_two->NeutralizePotential(id_two, 1); // -1 increase potential.
  }
  return true;
}

void Field::PrintField(Player* player, Player* enemy) {
  std::shared_lock sl_field(mutex_field_);
  auto field = field_;
  sl_field.unlock();

  UpdateField(player, field);
  UpdateField(enemy, field);

  for (int l=0; l<lines_; l++) {
    for (int c=0; c<cols_; c++) {
      position_t cur = {l, c};
      // highlight -> magenta
      if (std::find(highlight_.begin(), highlight_.end(), cur) != highlight_.end())
        attron(COLOR_PAIR(COLOR_HIGHLIGHT));
      else if (std::find(blinks_.begin(), blinks_.end(), cur) != blinks_.end())
        attron(COLOR_PAIR(COLOR_HIGHLIGHT));
      // IPSP is on enemy neuron -> cyan.
      else if (player->IsNeuronBlocked(cur) || enemy->IsNeuronBlocked(cur))
          attron(COLOR_PAIR(COLOR_RESOURCES));
      // both players -> cyan
      else if (CheckCollidingPotentials(cur, player, enemy))
        attron(COLOR_PAIR(COLOR_RESOURCES));
      // Resource
      else if (enemy->GetNeuronTypeAtPosition(cur) == RESOURCENEURON 
          || player->GetNeuronTypeAtPosition(cur) == RESOURCENEURON)
        attron(COLOR_PAIR(COLOR_RESOURCES));
      // player 2 -> red
      else if (enemy->GetNeuronTypeAtPosition(cur) != -1 || enemy->GetPotentialIdIfPotential(cur) != "")
        attron(COLOR_PAIR(COLOR_PLAYER));
      // player 1 -> blue 
      else if (player->GetNeuronTypeAtPosition(cur) != -1 || player->GetPotentialIdIfPotential(cur) != "")
        attron(COLOR_PAIR(COLOR_KI));
      // range -> green
      else if (InRange(cur, range_, range_center_) 
          && player->GetNeuronTypeAtPosition(cur) != UnitsTech::NUCLEUS)
        attron(COLOR_PAIR(COLOR_OK));
      // Replace certain elements.
      if (replacements_.count(cur) > 0)
        mvaddch(15+l, left_border_ + 2*c, replacements_.at(cur));
      else
        mvaddstr(15+l, left_border_ + 2*c, field[l][c].c_str());
      mvaddch(15+l, left_border_ + 2*c+1, ' ' );
      attron(COLOR_PAIR(COLOR_DEFAULT));
    }
  }
  blinks_.clear();
}

bool Field::InRange(position_t pos, int range, position_t start) {
  if (range == ViewRange::GRAPH)
    return graph_.InGraph(pos);
  return utils::Dist(pos, start) <= range;
}

bool Field::InField(int l, int c) {
  return (l >= 0 && l <= lines_ && c >= 0 && c <= cols_);
}

position_t Field::FindFree(int l, int c, int min, int max) {
  std::shared_lock sl_field(mutex_field_);

  auto positions = GetAllInRange({l, c}, max, min, true);
  if (positions.size() == 0)
    throw std::runtime_error("Game came to an strange end. No free positions!");

  return positions[ran_gen_->RandomInt(0, positions.size())];
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
  position_t upper_corner = {start.first-max_dist, start.second-max_dist};
  for (int i=0; i<=max_dist*2; i++) {
    for (int j=0; j<=max_dist*2; j++) {
      position_t pos = {upper_corner.first+i, upper_corner.second+j};
      if (InField(pos.first, pos.second) && utils::InRange(start, pos, min_dist, max_dist)
          && (!free || field_[pos.first][pos.second] == SYMBOL_FREE)
          && (!free || graph_.InGraph(pos)))
        positions_in_range.push_back(pos);
    }
  }
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
