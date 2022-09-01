#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <curses.h>
#include <exception>
#include <iostream>
#include <iterator>
#include <locale>
#include <memory>
#include <random>
#include <stdexcept>
#include <vector>

#include "share/defines.h"
#include "share/shemes/data.h"
#include "share/tools/graph.h"
#include "spdlog/spdlog.h"

#include "server/game/field/field.h"
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
  graph_ = std::make_shared<Graph>();

  // initialize empty field.
  for (int l=0; l<=lines_; l++) {
    field_.push_back({});
    for (int c=0; c<=cols_; c++)
      field_[l].push_back(SYMBOL_FREE);
  }
}

Field::Field(const Field& field) {
  lines_ = field.lines_;
  cols_ = field.cols_;
  ran_gen_ = field.ran_gen_; // check deep-copy
  graph_ = field.graph_; 
  field_ = field.field_; 
  // neurons are added when player's neurons are added.
}

// getter 
int Field::lines() { return lines_; }
int Field::cols() { return cols_; }
std::shared_ptr<Graph> Field::graph() { return graph_; }
const std::map<position_t, std::map<int, position_t>>& Field::resource_neurons() { return resource_neurons_; }
const std::map<position_t, std::vector<std::pair<std::string, Player*>>>& Field::epsps() { return epsps_; }

std::vector<position_t> Field::GraphPositions() {
  std::vector<position_t> graph_positions;
  for (const auto& it : graph_->nodes())
    graph_positions.push_back(it.second->pos_);
  return graph_positions;
}

std::vector<position_t> Field::AddNucleus(int num_players) {
  spdlog::get(LOGGER)->info("Field::AddNucleus: num players {}", num_players);
  // Get all availible sections (more than a fourth of the positions of a section exist in graph). 
  std::vector<int> availible_sections;
  for (int i=1; i<=8; i++) {
    int free_positions = 0;
    auto positions_in_section = GetAllPositionsOfSection(i);
    for (const auto& it : positions_in_section) {
      if (graph_->InGraph(it)) 
        free_positions++;
    }
    if (free_positions > (int)positions_in_section.size()/4)
      availible_sections.push_back(i);
  }
  // If not at least half as many sections are availible as players (max 2 players per section) omit.
  if ((int)availible_sections.size() < num_players/2)
    return {};
  // Create nucleus-positions.
  std::vector<position_t> nucleus_positions;
  for (int i=0; i<num_players; i++) {
    // Get random section and then erase retreived section from availible sections
    int section = ran_gen_->RandomInt(0, availible_sections.size()-1);
    auto positions_in_section = GetAllPositionsOfSection(availible_sections[section], true);
    // availible_sections.erase(availible_sections.begin() + section);
    // Make sure no other nucleus is too near and enough free fields
    position_t pos = positions_in_section[ran_gen_->RandomInt(0, positions_in_section.size()-1)];
    while (NucleusInRange(pos, 8) || GetAllInRange(pos, 1.5, 0, true).size() < 8)
      pos = positions_in_section[ran_gen_->RandomInt(0, positions_in_section.size())];
    // Add to field and nucleus positions.
    field_[pos.first][pos.second] = SYMBOL_DEN;
    nucleus_positions.push_back(pos);
    AddResources(pos);
    spdlog::get(LOGGER)->info("Adding nucleus at position: {}", utils::PositionToString(pos));
  }
  spdlog::get(LOGGER)->info("Field::AddNucleus: done");
  return nucleus_positions;
}

void Field::AddResources(position_t start_pos) {
  spdlog::get(LOGGER)->info("Field::AddResources {}", utils::PositionToString(start_pos));
  std::map<int, position_t> resource_positions;

  // Get positions sourrounding start position, with enough free spaces for all resources.
  int nth_try = 0;
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
  resource_neurons_[start_pos] = resource_positions;
  spdlog::get(LOGGER)->info("Field::AddResources: done");
}

void Field::BuildGraph() {
  spdlog::get(LOGGER)->info("Field::BuildGraph");
  // Add all nodes.
  spdlog::get(LOGGER)->debug("Field::BuildGraph: Adding nodes...");
  for (int l=0; l<lines_; l++) {
    for (int c=0; c<cols_; c++) {
      if (field_[l][c] != SYMBOL_HILL)
        graph_->AddNode(l, c);
    }
  }
  // For each node, add edges.
  spdlog::get(LOGGER)->debug("Field::BuildGraph: Adding edged...");
  for (auto node : graph_->nodes()) {
    for (const auto& pos : GetAllInRange(node.second->pos_, 1.5, 1)) {
      if (InField(pos) && field_[pos.first][pos.second] != SYMBOL_HILL && graph_->InGraph(pos))
        graph_->AddEdge(node.second, graph_->GetNode(pos));
    }
  }
  // Remove all nodes not in main circle
  spdlog::get(LOGGER)->debug("Field::BuildGraph: reducing to greates component...");
  int num_positions_before = graph_->nodes().size();
  graph_->ReduceToGreatestComponent();
  spdlog::get(LOGGER)->info("Field::BuildGraph: Done. {}/{} positions left!", graph_->nodes().size(), 
      num_positions_before);
}

void Field::AddHills(std::vector<double> reduced_pitches, double avrg_pitch, int looseness) {
  // reverse pitches to use `.back()` and `.pop_back()` 
  std::reverse(reduced_pitches.begin(), reduced_pitches.end());
  // Iterate over field and create 'hill', small or big 'mountain' based on matching pitch
  for (int l=0; l<lines_; l++) {
    for (int c=0; c<cols_; c++) {
      // small hill: slightly below average pitch (low looseness influence)
      if (reduced_pitches.back() < (avrg_pitch/(1.8+looseness*0.1)))
        field_[l][c] = SYMBOL_HILL;
      // small hill: below average pitch (medium looseness influence)
      if (reduced_pitches.back() < avrg_pitch/(3.5+looseness*0.8))
        for (const auto& it : GetAllInRange({l, c}, 1.5, 0))
          field_[it.first][it.second] = SYMBOL_HILL;
      // small hill: obviously below average pitch (high looseness influence)
      if (reduced_pitches.back() < avrg_pitch/(4.1+looseness*2))
        for (const auto& it : GetAllInRange({l, c}, 2, 1))
          field_[it.first][it.second] = SYMBOL_HILL;
      reduced_pitches.pop_back();
    }
  }
}


std::list<position_t> Field::GetWay(position_t start_pos, const std::vector<position_t>& way_points) {
  spdlog::get(LOGGER)->debug("Field::GetWayForSoldier: pos={}", utils::PositionToString(start_pos));
  if (way_points.size() < 1)
    return {};
  std::list<position_t> way = {start_pos};
  for (size_t i=0; i<way_points.size(); i++) {
    try {
      // Get new way-part
      auto new_part = graph_->DijkstrasWay(way.back(), way_points[i]);
      way.insert(way.end(), std::next(new_part.begin()), new_part.end());
      // set start-pos as current way-point
      start_pos = way_points[0];
    }
    catch (std::exception& e) {
      spdlog::get(LOGGER)->error("Field::GetWayForSoldier: Serious error: no way found: {}", e.what());
    }
  }
  return way;
}

void Field::AddNewNeuron(position_t pos, std::shared_ptr<Neuron> neuron, Player* p) {
  spdlog::get(LOGGER)->debug("Field::AddNewUnitToPos({})", utils::PositionToString(pos));
  // Add unit to field if in unit-symbol-mapping (resource-neurons already exist)
  if (unit_symbol_mapping.count(neuron->type_) > 0)
    field_[pos.first][pos.second] = unit_symbol_mapping.at(neuron->type_);
  // Add to neurons
  neurons_[pos] = {neuron, p};
}

void Field::RemoveNeuron(position_t pos) {
  if (neurons_.count(pos) > 0)
    neurons_.erase(pos);
}

std::pair<int, Player*> Field::GetNeuronTypeAndPlayerAtPosition(position_t pos) {
  if (neurons_.count(pos) > 0)
    return {neurons_.at(pos).first->type_, neurons_.at(pos).second};
  return {-1, nullptr};
}

bool Field::InField(position_t pos) {
  return (pos.first >= 0 && pos.first <= lines_ && pos.second >= 0 && pos.second <= cols_);
}

position_t Field::FindFree(position_t pos, int min, int max) {
  auto positions = GetAllInRange(pos, max, min, true);
  if (positions.size() == 0)
    return DEFAULT_POS;
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
          && (!free || graph_->InGraph(pos)))
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

std::vector<position_t> Field::GetAllPositionsOfSection(int interval, bool in_graph) {
  std::vector<position_t> positions;
  int l = (interval-1)%(SECTIONS/2)*(cols_/4);
  int c = (interval < (SECTIONS/2)+1) ? 0 : lines_/2;
  for (int i=l; i<l+cols_/4; i++) {
    for (int j=c; j<c+lines_/2; j++) {
      if (!in_graph || graph_->InGraph({j, i}))
        positions.push_back({j, i});
    }
  }
  return positions;
}

std::vector<std::vector<Data::Symbol>> Field::Export(std::map<std::string, Player*> players) {
  spdlog::get(LOGGER)->debug("Field::Export");
  std::vector<Player*> vec_players;
  for (const auto& it : players)
    vec_players.push_back(it.second);
  return Export(vec_players);
}

std::vector<std::vector<Data::Symbol>> Field::Export(std::vector<Player*> players) {
  spdlog::get(LOGGER)->debug("Field::Export");
  std::vector<std::vector<Data::Symbol>> t_field;
  // Create transfer-type field.
  for (int l=0; l<lines_; l++) {
    t_field.push_back(std::vector<Data::Symbol>());
    for (int c=0; c<cols_; c++) {
      position_t cur = {l, c};
      int color = COLOR_DEFAULT;
      // Check if belongs to either player, is blocked or is resource-neuron
      for (size_t i=0; i<players.size(); i++) {
        auto new_color = players[i]->GetColorForPos(cur);
        if (new_color != COLOR_DEFAULT)
          color = new_color;
      }
      // Add to json field.
      t_field[l].push_back(Data::Symbol({field_[l][c], color}));
    }
  }
  spdlog::get(LOGGER)->debug("Field::Export done");
  return t_field;
}

bool Field::NucleusInRange(position_t pos, int range) {
  for (const auto& it : GetAllInRange(pos, range, 0))
    if (field_[it.first][it.second] == SYMBOL_DEN)
      return true;
  return false; 
}

void Field::GatherEpspsFromPlayers(std::map<std::string, Player*> players) {
  epsps_.clear();
  for (const auto& p : players) {
    for (const auto& potential : p.second->potential()) {
      if (potential.first.find("epsp") != std::string::npos || potential.first.find("macro_1") != std::string::npos)
        epsps_[potential.second.pos_].push_back({potential.first, p.second});
    }
  }
}
