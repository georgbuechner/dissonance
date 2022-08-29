#include "share/shemes/data.h"
#include "share/constants/codes.h"
#include "share/defines.h"
#include "share/tools/utils/utils.h"
#include <cstddef>
#include <memory>
#include <msgpack.hpp>
#include <string>
#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"

// SYMBOL 
Data::Symbol::Symbol() {};
Data::Symbol::Symbol(std::pair<std::string, short> symbol) {
  symbol_ = symbol.first;
  color_ = symbol.second;
}

std::string Data::Symbol::str() const { 
  return "Symbol: " + symbol_ + "(color: " + std::to_string(color_) + ")";
}

std::pair<std::string, short> Data::Symbol::to_pair() const {
  return {symbol_, color_};
}

// RESOUCE 
Data::Resource::Resource() {}
Data::Resource::Resource(double value, double bound, unsigned int limit, unsigned int iron, bool active) {
  value_ = utils::Dtos(value);
  bound_ = utils::Dtos(bound);
  limit_ = std::to_string(limit);
  iron_ = std::to_string(iron);
  active_ = active;
}

Data::Resource::Resource(const char* payload, size_t len, size_t& offset) {
  msgpack::object_handle result;
  unpack(result, payload, len, offset);
  value_ = result->as<std::string>();
  unpack(result, payload, len, offset);
  bound_ = result->as<std::string>();
  unpack(result, payload, len, offset);
  limit_ = result->as<std::string>();
  unpack(result, payload, len, offset);
  iron_ = result->as<std::string>();
  unpack(result, payload, len, offset);
  active_ = result->as<bool>();
}

void Data::Resource::binary(std::stringstream& buffer) const {
  msgpack::pack(buffer, value_);
  msgpack::pack(buffer, bound_);
  msgpack::pack(buffer, limit_);
  msgpack::pack(buffer, iron_);
  msgpack::pack(buffer, active_);
}

// DATA
Data::Data() {}

// LOBBY ENTRY
Data::LobbyEntry::LobbyEntry(short max_players, short cur_players, std::string game_id, std::string audio_map_name) {
  _max_players = max_players;
  _cur_players = cur_players;
  _game_id = game_id;
  _audio_map_name = audio_map_name;
}

Data::LobbyEntry::LobbyEntry(const char* payload, size_t len, size_t& offset) {
  msgpack::object_handle result;
  unpack(result, payload, len, offset);
  _max_players = result->as<short>();
  unpack(result, payload, len, offset);
  _cur_players = result->as<short>();
  unpack(result, payload, len, offset);
  _game_id = result->as<std::string>();
  unpack(result, payload, len, offset);
  _audio_map_name= result->as<std::string>();
}

void Data::LobbyEntry::binary(std::stringstream& buffer) const {
  msgpack::pack(buffer, _max_players);
  msgpack::pack(buffer, _cur_players);
  msgpack::pack(buffer, _game_id);
  msgpack::pack(buffer, _audio_map_name);
}


// MESSAGE

Msg::Msg(std::string msg) : Data(), msg_(msg) {}
Msg::Msg(const char* payload, size_t len, size_t& offset) : Data() {
  msgpack::object_handle result;
  unpack(result, payload, len, offset);
  msg_ = result->as<std::string>();
}

// getter 
std::string Msg::msg() { return msg_; }

// methods 
void Msg::binary(std::stringstream& buffer) {
  msgpack::pack(buffer, msg_);
}

// SELECT MODE
InitNewGame::InitNewGame(unsigned short mode, unsigned short lines, unsigned short cols) : Data() {
  mode_ = mode;
  lines_ = lines;
  cols_ = cols;
  num_players_ = 0;
  game_id_ = "";
}

InitNewGame::InitNewGame(const char* payload, size_t len, size_t& offset) : Data() {
  msgpack::object_handle result;
  unpack(result, payload, len, offset);
  mode_ = result->as<unsigned short>();
  unpack(result, payload, len, offset);
  lines_ = result->as<unsigned short>();
  unpack(result, payload, len, offset);
  cols_= result->as<unsigned short>();
  unpack(result, payload, len, offset);
  num_players_ = result->as<unsigned short>();
  unpack(result, payload, len, offset);
  game_id_ = result->as<std::string>();
}

// getter
unsigned short InitNewGame::mode() { return mode_; }
unsigned short InitNewGame::lines() { return lines_; }
unsigned short InitNewGame::cols() { return cols_; }
unsigned short InitNewGame::num_players() { return num_players_; }
std::string InitNewGame::game_id() { return game_id_; }

// setter 
void InitNewGame::set_num_players(unsigned short num_players) { num_players_ = num_players; }
void InitNewGame::set_game_id(std::string game_id) { game_id_ = game_id; }

// methods 
void InitNewGame::binary(std::stringstream& buffer) {
  msgpack::pack(buffer, mode_);
  msgpack::pack(buffer, lines_);
  msgpack::pack(buffer, cols_);
  msgpack::pack(buffer, num_players_);
  msgpack::pack(buffer, game_id_);
}

// UPDATE_GAME
Update::Update(std::map<std::string, std::pair<std::string, int>> players, 
    std::map<position_t, std::pair<std::string, short>> potentials,
    std::map<position_t, int> new_dead_neurons,
    float audio_played) 
  : Data(), players_(players), potentials_(potentials), new_dead_neurons_(new_dead_neurons), 
    audio_played_(audio_played) {};

Update::Update(const char* payload, size_t len, size_t& offset) : Data() {
  msgpack::object_handle result;

  unpack(result, payload, len, offset);
  players_ = result->as<std::map<std::string, std::pair<std::string, int>>>();

  unpack(result, payload, len, offset);
  potentials_ = result->as<std::map<position_t, std::pair<std::string, short>>>();

  unpack(result, payload, len, offset);
  new_dead_neurons_ = result->as<std::map<position_t, int>>();
  spdlog::get(LOGGER)->debug("New-dead-neurons size: {}", new_dead_neurons_.size());

  unpack(result, payload, len, offset);
  audio_played_ = result->as<float>();

  unpack(result, payload, len, offset);
  unsigned short num_resources = result->as<unsigned short>();
  for (unsigned short i=0; i<num_resources; i++) {
    unpack(result, payload, len, offset);
    int resource = result->as<int>();
    resources_[resource] = Resource(payload, len, offset);
  }

  unpack(result, payload, len, offset);
  build_options_ = result->as<std::vector<bool>>();

  unpack(result, payload, len, offset);
  synapse_options_ = result->as<std::vector<bool>>();
}

// getter 
std::map<std::string, std::pair<std::string, int>> Update::players() { return players_; }
std::map<position_t, std::pair<std::string, short>> Update::potentials() { return potentials_; }
std::map<position_t, int> Update::new_dead_neurons() { return new_dead_neurons_; }
float Update::audio_played() { return audio_played_; }
std::map<int, Data::Resource> Update::resources() { return resources_; }
std::vector<bool> Update::build_options() { return build_options_; }
std::vector<bool> Update::synapse_options() { return synapse_options_; }

// setter 
void Update::set_resources(std::map<int, Data::Resource> resources) { resources_ = resources; }
void Update::set_build_options(std::vector<bool> build_options) { build_options_ = build_options; }
void Update::set_synapse_options(std::vector<bool> synapse_options) { synapse_options_ = synapse_options; }

// methods 
void Update::binary(std::stringstream& buffer) {
  msgpack::pack(buffer, players_);
  msgpack::pack(buffer, potentials_);
  msgpack::pack(buffer, new_dead_neurons_);
  spdlog::get(LOGGER)->debug("New-dead-neurons size: {}", new_dead_neurons_.size());
  msgpack::pack(buffer, audio_played_);
  // resources
  msgpack::pack(buffer, resources_.size());
  for (const auto& resource : resources_) {
    msgpack::pack(buffer, resource.first);
    resource.second.binary(buffer);
  }
  msgpack::pack(buffer, build_options_);
  msgpack::pack(buffer, synapse_options_);
}

t_topline Update::PlayersToPrint() {
  t_topline print;
  for (const auto& it : players_) {
    print.push_back({it.first + ": " + it.second.first, it.second.second});
    print.push_back({" | ", COLOR_DEFAULT});
  }
  if (print.size() > 0)
    print.pop_back();
  return print;
}

// INIT 
Init::Init(std::shared_ptr<Update> update, std::vector<std::vector<Data::Symbol>> field, 
   std::vector<position_t> graph_positions, std::map<int, tech_of_t> technologies) : Data() {
  field_ = field;
  graph_positions_ = graph_positions;
  technologies_ = technologies;
  update_ = update;
}

Init::Init(const char* payload, size_t len, size_t& offset) : Data() {
  msgpack::object_handle result;

  // Create field.
  unpack(result, payload, len, offset);
  std::vector<std::vector<std::pair<std::string, short>>> field = 
    result->as<std::vector<std::vector<std::pair<std::string, short>>> >();
  // Convert field back to std-field
  for (const auto& it : field) {
    std::vector<Symbol> row;
    for (const auto& jt : it)
      row.push_back(Symbol({jt.first, jt.second}));
    field_.push_back(row);
  }

  unpack(result, payload, len, offset);
  graph_positions_ = result->as<std::vector<position_t>>();
  unpack(result, payload, len, offset);
  technologies_ = result->as<std::map<int, tech_of_t>>();
  unpack(result, payload, len, offset);
  macro_ = result->as<short>();
  unpack(result, payload, len, offset);
  ai_strategies_ = result->as<std::map<std::string, size_t>>();

  // Create update-object
  update_ = std::make_shared<Update>(payload, len, offset);
}

// getter 
std::vector<std::vector<Data::Symbol>> Init::field() { return field_; }
std::vector<position_t> Init::graph_positions() { return graph_positions_; }
std::map<int, tech_of_t> Init::technologies() { return technologies_; }
std::shared_ptr<Update> Init::update() { return update_; }
short Init::macro() { return macro_; }
std::map<std::string, size_t> Init::ai_strategies() { return ai_strategies_; }

// setter 
void Init::set_macro(short macro) { macro_ = macro; }
void Init::set_ai_strategies(std::map<std::string, size_t> def_strategies) { ai_strategies_ = def_strategies; }

// methods 
void Init::binary(std::stringstream& buffer) { 
  // convert field for easier endocding 
  std::vector<std::vector<std::pair<std::string, short>>> field;
  for (const auto& it : field_) {
    std::vector<std::pair<std::string, short>> row;
    for (const auto& jt : it)
      row.push_back({jt.symbol_, jt.color_});
    field.push_back(row);
  }
  msgpack::pack(buffer, field);
  msgpack::pack(buffer, graph_positions_);
  msgpack::pack(buffer, technologies_);
  msgpack::pack(buffer, macro_);
  msgpack::pack(buffer, ai_strategies_);
  update_->binary(buffer);
}


// SET_UNIT 
FieldPosition::FieldPosition(position_t pos, short unit, short color) 
  : Data(), pos_(pos), unit_(unit), color_(color), resource_(0xFFF) {}
FieldPosition::FieldPosition(position_t pos, short unit, short color, unsigned short resource) 
  : Data(), pos_(pos), unit_(unit), color_(color), resource_(resource) {}

FieldPosition::FieldPosition(const char* payload, size_t len, size_t& offset) : Data() {
  msgpack::object_handle result;

  unpack(result, payload, len, offset);
  pos_ = result->as<position_t>();

  unpack(result, payload, len, offset);
  unit_ = result->as<short>();

  unpack(result, payload, len, offset);
  color_ = result->as<short>();

  unpack(result, payload, len, offset);
  resource_ = result->as<unsigned short>();
}

// getter 
position_t FieldPosition::pos() const { return pos_; }
short FieldPosition::unit() const { return unit_; }
short FieldPosition::color() const { return color_; }
unsigned short FieldPosition::resource() const { return resource_; }

// methods 
void FieldPosition::binary(std::stringstream& buffer) {
  msgpack::pack(buffer, pos_);
  msgpack::pack(buffer, unit_);
  msgpack::pack(buffer, color_);
  msgpack::pack(buffer, resource_);
}

// SET_UNITS

Units::Units(std::vector<FieldPosition> units) : Data(), units_(units) {}
Units::Units(const char* payload, size_t len, size_t& offset) : Data() {
  while (offset != len) {
    units_.push_back(FieldPosition(payload, len, offset));
  }
}

// getter 
std::vector<FieldPosition> Units::units() const { return units_; }

// methods 
void Units::binary(std::stringstream& buffer) {
  for (auto& it : units_) {
    it.binary(buffer);
  }
}

// LOBBY
Lobby::Lobby() { lobby_ = {}; }
Lobby::Lobby(const char* payload, size_t len, size_t& offset) : Data() {
  while (offset != len) {
    lobby_.push_back(LobbyEntry(payload, len, offset));
  }
}

// getter 
const std::vector<Data::LobbyEntry> Lobby::lobby() { return lobby_; }

// methods    
void Lobby::AddEntry(std::string game_id, short max_players, short cur_players, std::string audio_map_name) {
  lobby_.push_back(LobbyEntry({max_players, cur_players, game_id, audio_map_name}));
}

void Lobby::clear() {
  lobby_.clear();
}

void Lobby::binary(std::stringstream& buffer) {
  for (auto& it : lobby_) {
    it.binary(buffer);
  }
}

// STATISTICS
Statictics::Statictics() : Data(), player_color_(0), epsp_swallowed_(0) {}
Statictics::Statictics(const char* payload, size_t len, size_t& offset) : Data() {
  msgpack::object_handle result;

  unpack(result, payload, len, offset);
  player_name_ = result->as<std::string>();
  unpack(result, payload, len, offset);
  player_color_ = result->as<int>();
  unpack(result, payload, len, offset);
  neurons_build_ = result->as<std::map<unsigned short, unsigned short>>();
  unpack(result, payload, len, offset);
  potentials_build_ = result->as<std::map<unsigned short, unsigned short>>();
  unpack(result, payload, len, offset);
  potentials_killed_ = result->as<std::map<unsigned short, unsigned short>>();
  unpack(result, payload, len, offset);
  potentials_lost_ = result->as<std::map<unsigned short, unsigned short>>();
  unpack(result, payload, len, offset);
  epsp_swallowed_ = result->as<unsigned short>();
  unpack(result, payload, len, offset);
  resources_ = result->as<std::map<int, std::map<std::string, double>>>();
  unpack(result, payload, len, offset);
  technologies_ = result->as<std::map<int, tech_of_t>>();
  unpack(result, payload, len, offset);
  size_t graph_size = result->as<size_t>();
  for (size_t i=0; i<graph_size; i++) 
    graph_.push_back(StaticticsEntry(payload, len, offset));
}
Statictics::Statictics(const Statictics& statistics) {
  player_name_ = statistics.player_name_;
  player_color_ = statistics.player_color_;
  neurons_build_ = statistics.neurons_build_;
  potentials_build_ = statistics.potentials_build_;
  potentials_killed_ = statistics.potentials_killed_;
  potentials_lost_ = statistics.potentials_lost_;
  epsp_swallowed_ = statistics.epsp_swallowed_;
  resources_ = statistics.resources_;
  technologies_ = statistics.technologies_;
}

// getter
std::string Statictics::player_name() const { return player_name_; }
short Statictics::player_color() const { return player_color_; }
std::map<unsigned short, unsigned short> Statictics::neurons_build() const { return neurons_build_; }
std::map<unsigned short, unsigned short> Statictics::potentials_build() const { return potentials_build_; }
std::map<unsigned short, unsigned short> Statictics::potentials_killed() const { return potentials_killed_; }
std::map<unsigned short, unsigned short> Statictics::potentials_lost() const { return potentials_lost_; }
unsigned short Statictics::epsp_swallowed() const { return epsp_swallowed_; }
std::map<int, std::map<std::string, double>> Statictics::stats_resources() const { return resources_; }
std::map<int, tech_of_t> Statictics::stats_technologies() const { return technologies_; }
std::map<int, std::map<std::string, double>>& Statictics::stats_resources_ref() { return resources_; }
std::vector<StaticticsEntry> Statictics::graph() const { return graph_; }
// setter
void Statictics::set_player_name(std::string player_name) { player_name_ = player_name; }
void Statictics::set_color(short color) { player_color_ = color; }
void Statictics::set_technologies(std::map<int, tech_of_t> technologies) { technologies_ = technologies; }

// methods
void Statictics::AddNewNeuron(int unit) {
  neurons_build_[unit]++;
  tmp_neurons_built_.push_back(unit); // add neuron built to tmp neurons, so it can be added to graph.
}
void Statictics::AddNewPotential(int unit) {
  potentials_build_[unit]++;
}
void Statictics::AddKillderPotential(std::string id) {
  int unit = (id.find("ipsp") != std::string::npos) ? IPSP : EPSP;
  potentials_killed_[unit]++;
}
void Statictics::AddLostPotential(std::string id) {
  int unit = (id.find("ipsp") != std::string::npos) ? IPSP : EPSP;
  potentials_lost_[unit]++;
}
void Statictics::AddEpspSwallowed() {
  epsp_swallowed_++;
}
void Statictics::AddStatisticsEntry(double oxygen, double potassium, double chloride, double glutamate, 
        double dopamine, double serotonin) {
  graph_.push_back(StaticticsEntry(oxygen, potassium, chloride, glutamate, dopamine, serotonin, tmp_neurons_built_));
  tmp_neurons_built_.clear(); // clear tmp neurons.
}

void Statictics::binary(std::stringstream& buffer) {
  msgpack::pack(buffer, player_name_);
  msgpack::pack(buffer, player_color_);
  msgpack::pack(buffer, neurons_build_);
  msgpack::pack(buffer, potentials_build_);
  msgpack::pack(buffer, potentials_killed_);
  msgpack::pack(buffer, potentials_lost_);
  msgpack::pack(buffer, epsp_swallowed_);
  msgpack::pack(buffer, resources_);
  msgpack::pack(buffer, technologies_);
  msgpack::pack(buffer, graph_.size());
  for (auto& it : graph_) 
    it.binary(buffer);
}

StaticticsEntry::StaticticsEntry(const char* payload, size_t len, size_t& offset) : Data() {
  msgpack::object_handle result;
  unpack(result, payload, len, offset);
  oxygen_ = result->as<double>();
  unpack(result, payload, len, offset);
  potassium_= result->as<double>();
  unpack(result, payload, len, offset);
  chloride_ = result->as<double>();
  unpack(result, payload, len, offset);
  glutamate_ = result->as<double>();
  unpack(result, payload, len, offset);
  dopamine_ = result->as<double>();
  unpack(result, payload, len, offset);
  serotonin_ = result->as<double>();
  unpack(result, payload, len, offset);
  neurons_built_ = result->as<std::vector<int>>();
}

StaticticsEntry::StaticticsEntry(double oxygen, double potassium, double chloride, double glutamate, 
    double dopamine, double serotonin, std::vector<int> neurons_built) : Data() {
  oxygen_ = oxygen;
  potassium_ = potassium;
  chloride_ = chloride;
  glutamate_ = glutamate;
  dopamine_ = dopamine;
  serotonin_ = serotonin;
  neurons_built_ = neurons_built;
}

// getter 
double StaticticsEntry::oxygen() { return oxygen_; }
double StaticticsEntry::potassium() { return potassium_; }
double StaticticsEntry::chloride() { return chloride_; }
double StaticticsEntry::glutamate() { return glutamate_; }
double StaticticsEntry::dopamine() { return dopamine_; }
double StaticticsEntry::serotonin() { return serotonin_; }
std::vector<int> StaticticsEntry::neurons_built() { return neurons_built_; }

// methods 
void StaticticsEntry::binary(std::stringstream& buffer) {
  msgpack::pack(buffer, oxygen_);
  msgpack::pack(buffer, potassium_);
  msgpack::pack(buffer, chloride_);
  msgpack::pack(buffer, glutamate_);
  msgpack::pack(buffer, dopamine_);
  msgpack::pack(buffer, serotonin_);
  msgpack::pack(buffer, neurons_built_);
}

GameEnd::GameEnd(std::string msg) : Data(), msg_(msg) {}
GameEnd::GameEnd(const char* payload, size_t len, size_t& offset) : Data() {
  msgpack::object_handle result;

  // get message
  unpack(result, payload, len, offset);
  msg_ = result->as<std::string>();

  // get statistics
  while (offset != len) {
    statistics_.push_back(std::make_shared<Statictics>(payload, len, offset));
  }
}

// getter 
std::string GameEnd::msg() { return msg_; }
std::vector<std::shared_ptr<Statictics>> GameEnd::statistics() { return statistics_; }

// setter
void GameEnd::set_msg(std::string msg) { msg_ = msg; }


// methods 
void GameEnd::AddStatistics(std::shared_ptr<Statictics> statistics) {
  statistics_.push_back(statistics);
}

void GameEnd::binary(std::stringstream& buffer) {
  msgpack::pack(buffer, msg_);
  for (auto& it : statistics_) {
    it->binary(buffer);
  }
}

// BUILD POTENTIAL 
BuildPotential::BuildPotential(short unit, short num) : Data(), unit_(unit), num_(num), synapse_pos_(DEFAULT_POS) {}
BuildPotential::BuildPotential(const char* payload, size_t len, size_t& offset) : Data() {
  msgpack::object_handle result;
  unpack(result, payload, len, offset);
  unit_ = result->as<short>();
  unpack(result, payload, len, offset);
  num_ = result->as<short>();
  unpack(result, payload, len, offset);
  synapse_pos_ = result->as<position_t>();
  unpack(result, payload, len, offset);
  positions_ = result->as<std::vector<position_t>>();
}

// getter 
short BuildPotential::unit() const { return unit_; }
short BuildPotential::num() { return num_; }
position_t BuildPotential::synapse_pos() { return synapse_pos_; }
std::vector<position_t> BuildPotential::positions() { return positions_; }

// setter 
void BuildPotential::set_start_pos(position_t pos) { synapse_pos_= pos; }
void BuildPotential::set_positions(std::vector<position_t> positions) { positions_ = positions; }

// methods
void BuildPotential::binary(std::stringstream& buffer) {
  msgpack::pack(buffer, unit_);
  msgpack::pack(buffer, num_);
  msgpack::pack(buffer, synapse_pos_);
  msgpack::pack(buffer, positions_);
}

void BuildPotential::SetPickedPosition(position_t pos) { synapse_pos_ = pos; }

// BUILD NEURON
BuildNeuron::BuildNeuron(short unit) : Data(), unit_(unit), range_(0), pos_(DEFAULT_POS), start_pos_(DEFAULT_POS) {}
BuildNeuron::BuildNeuron(const char* payload, size_t len, size_t& offset) : Data() {
  msgpack::object_handle result;
  unpack(result, payload, len, offset);
  unit_ = result->as<short>();
  unpack(result, payload, len, offset);
  range_ = result->as<short>();
  unpack(result, payload, len, offset);
  pos_ = result->as<position_t>();
  unpack(result, payload, len, offset);
  start_pos_ = result->as<position_t>();
  unpack(result, payload, len, offset);
  positions_ = result->as<std::vector<position_t>>();
}

// getter 
short BuildNeuron::unit() const { return unit_; }
position_t BuildNeuron::pos() const { return pos_; }
position_t BuildNeuron::start_pos() { return start_pos_; }
std::vector<position_t> BuildNeuron::positions() { return positions_; }
short BuildNeuron::range() { return range_; }

// setter 
void BuildNeuron::set_pos(position_t pos) { pos_ = pos; }
void BuildNeuron::set_start_pos(position_t pos) { start_pos_ = pos; }
void BuildNeuron::set_positions(std::vector<position_t> positions) { positions_ = positions; }
void BuildNeuron::set_range(short range) { range_ = range; }

// methods
void BuildNeuron::binary(std::stringstream& buffer) {
  msgpack::pack(buffer, unit_);
  msgpack::pack(buffer, range_);
  msgpack::pack(buffer, pos_);
  msgpack::pack(buffer, start_pos_);
  msgpack::pack(buffer, positions_);
}

void BuildNeuron::SetPickedPosition(position_t pos) { 
  if (start_pos_ == DEFAULT_POS)
    start_pos_ = pos;
  else 
    pos_ = pos;
}


// SELECT SYNAPSE
SelectSynapse::SelectSynapse() : Data(), synapse_pos_(DEFAULT_POS), player_units_({}) {}
SelectSynapse::SelectSynapse(position_t synapse_pos) : Data(), synapse_pos_(synapse_pos), player_units_({}) {}
SelectSynapse::SelectSynapse(std::vector<position_t> positions) : Data(), synapse_pos_(DEFAULT_POS),
  player_units_(positions) {}
SelectSynapse::SelectSynapse(const char* payload, size_t len, size_t& offset) : Data() {
  msgpack::object_handle result;
  unpack(result, payload, len, offset);
  synapse_pos_ = result->as<position_t>();
  unpack(result, payload, len, offset);
  player_units_ = result->as<std::vector<position_t>>();
}

// getter 
position_t SelectSynapse::synapse_pos() { return synapse_pos_; }
std::vector<position_t> SelectSynapse::player_units() { return player_units_; }

// setter 
void SelectSynapse::set_synapse_pos(position_t pos) { synapse_pos_ = pos; }
void SelectSynapse::set_player_units(std::vector<position_t> positions) { 
  if (positions.size() == 1)
    synapse_pos_ = positions.front();
  else
    player_units_ = positions; 
}

// methods
void SelectSynapse::binary(std::stringstream& buffer) { 
  msgpack::pack(buffer, synapse_pos_);
  msgpack::pack(buffer, player_units_);
}

/**
 * Used by Pick-Context to set selected ('picked') position. 
 * Function needs to exists for all data-structs dependant on Pick-Context.
 */
void SelectSynapse::SetPickedPosition(position_t pos) { synapse_pos_ = pos; }


// SET WAYPOINTS
SetWayPoints::SetWayPoints(position_t synapse_pos) : Data(), synapse_pos_(synapse_pos) {
  start_pos_ = DEFAULT_POS;
  way_point_ = DEFAULT_POS;
  msg_ = "";
  num_ = 1;
};
SetWayPoints::SetWayPoints(position_t synapse_pos, short num) 
  : Data(), synapse_pos_(synapse_pos), num_(num) {
  start_pos_ = DEFAULT_POS;
  way_point_ = DEFAULT_POS;
  msg_ = "";
}

SetWayPoints::SetWayPoints(const char* payload, size_t len, size_t& offset) : Data() {
  msgpack::object_handle result;

  unpack(result, payload, len, offset);
  synapse_pos_ = result->as<position_t>();
  unpack(result, payload, len, offset);
  start_pos_ = result->as<position_t>();
  unpack(result, payload, len, offset);
  way_point_ = result->as<position_t>();

  unpack(result, payload, len, offset);
  centered_positions_ = result->as<std::vector<position_t>>();
  unpack(result, payload, len, offset);
  current_way_ = result->as<std::vector<position_t>>();
  unpack(result, payload, len, offset);
  current_waypoints_ = result->as<std::vector<position_t>>();

  unpack(result, payload, len, offset);
  msg_ = result->as<std::string>();
  unpack(result, payload, len, offset);
  num_ = result->as<short>();
}

// getter 
position_t SetWayPoints::synapse_pos() { return synapse_pos_; }
position_t SetWayPoints::start_pos() { return start_pos_; }
position_t SetWayPoints::way_point() { return way_point_; }
std::vector<position_t> SetWayPoints::centered_positions() { return centered_positions_; }
std::vector<position_t> SetWayPoints::current_way() { return current_way_; }
std::vector<position_t> SetWayPoints::current_waypoints() { return current_waypoints_; }
std::string SetWayPoints::msg() { return msg_; }
short SetWayPoints::num() { return num_; }

// setter 
void SetWayPoints::set_way_point(position_t pos) { way_point_ = pos; }
void SetWayPoints::set_centered_positions(std::vector<position_t> positions) { centered_positions_ = positions; }
void SetWayPoints::set_current_way(std::vector<position_t> positions) { current_way_ = positions; }
void SetWayPoints::set_current_waypoints(std::vector<position_t> positions) { current_waypoints_ = positions; }
void SetWayPoints::set_msg(std::string msg) { msg_ = msg; }
void SetWayPoints::set_num(short num) { num_ = num; }

// methods
void SetWayPoints::binary(std::stringstream& buffer) {
  msgpack::pack(buffer, synapse_pos_);
  msgpack::pack(buffer, start_pos_);
  msgpack::pack(buffer, way_point_);
  msgpack::pack(buffer, centered_positions_);
  msgpack::pack(buffer, current_way_);
  msgpack::pack(buffer, current_waypoints_);
  msgpack::pack(buffer, msg_);
  msgpack::pack(buffer, num_);
}

void SetWayPoints::SetPickedPosition(position_t pos) { 
  if (start_pos_ == DEFAULT_POS) 
    start_pos_ = pos;
  else
    way_point_ = pos; 
}

// SET TARGET 
SetTarget::SetTarget(position_t synapse_pos, short unit) : Data(), unit_(unit), synapse_pos_(synapse_pos) {
  start_pos_ = {-1, -1};
  target_ = {-1, -1};
}
SetTarget::SetTarget(const char* payload, size_t len, size_t& offset) : Data() {
  msgpack::object_handle result;

  unpack(result, payload, len, offset);
  unit_ = result->as<short>();
  unpack(result, payload, len, offset);
  synapse_pos_ = result->as<position_t>();
  unpack(result, payload, len, offset);
  start_pos_ = result->as<position_t>();
  unpack(result, payload, len, offset);
  target_ = result->as<position_t>();
  unpack(result, payload, len, offset);
  enemy_units_ = result->as<std::vector<position_t>>();
  unpack(result, payload, len, offset);
  target_positions_ = result->as<std::vector<position_t>>();
}

// getter 
short SetTarget::unit() const { return unit_; }
position_t SetTarget::synapse_pos() { return synapse_pos_; }
position_t SetTarget::start_pos() { return start_pos_; }
position_t SetTarget::target() { return target_; }
std::vector<position_t> SetTarget::enemy_units() { return enemy_units_; }
std::vector<position_t> SetTarget::target_positions() { return target_positions_; }

// setter 
void SetTarget::set_start_pos(position_t pos) { start_pos_ = pos; }
void SetTarget::set_target(position_t pos) { target_ = pos; }
void SetTarget::set_enemy_units(std::vector<position_t> positions) { 
  enemy_units_ = positions; 
  if (enemy_units_.size() == 1)
    start_pos_ = enemy_units_.front();
}
void SetTarget::set_target_positions(std::vector<position_t> positions) { target_positions_ = positions; }

// methods
void SetTarget::binary(std::stringstream& buffer) {
  msgpack::pack(buffer, unit_);
  msgpack::pack(buffer, synapse_pos_);
  msgpack::pack(buffer, start_pos_);
  msgpack::pack(buffer, target_);
  msgpack::pack(buffer, enemy_units_);
  msgpack::pack(buffer, target_positions_);
}

void SetTarget::SetPickedPosition(position_t pos) { 
  if (start_pos_ == DEFAULT_POS) 
    start_pos_ = pos; 
  else 
    target_ = pos;
}

// TOGGLE SWARM ATTACK
ToggleSwarmAttack::ToggleSwarmAttack(position_t synapse_pos) : Data(), synapse_pos_(synapse_pos) {}
ToggleSwarmAttack::ToggleSwarmAttack(const char* payload, size_t len, size_t& offset) : Data() {
  msgpack::object_handle result;
  unpack(result, payload, len, offset);
  synapse_pos_ = result->as<position_t>();
}

// getter 
position_t ToggleSwarmAttack::synapse_pos() { return synapse_pos_; }

// methods
void ToggleSwarmAttack::binary(std::stringstream& buffer) {
  msgpack::pack(buffer, synapse_pos_);
}

// GET POSITION INFOS

GetPositions::GetPositions(std::string return_cmd, std::map<short, PositionInfo> position_requests, 
    std::shared_ptr<Data> data) : Data(), return_cmd_(return_cmd), position_requests_(position_requests), data_(data) {
}

GetPositions::GetPositions(const char* payload, size_t len, size_t& offset) : Data() {
  msgpack::object_handle result;

  // Read return_cmd
  spdlog::get(LOGGER)->debug("GetPositions::GetPositions: reading return_cmd");
  unpack(result, payload, len, offset);
  return_cmd_ = result->as<std::string>();

  // Read num of position requests
  spdlog::get(LOGGER)->debug("GetPositions::GetPositions: reading num_requests");
  unpack(result, payload, len, offset);
  unsigned int num_requests = result->as<unsigned int>();
  // Read position requests
  for (unsigned int i=0; i<num_requests; i++) {
    // Read key
    spdlog::get(LOGGER)->debug("GetPositions::GetPositions: reading key");
    unpack(result, payload, len, offset);
    short key = result->as<short>();
    // Read and convert value
    spdlog::get(LOGGER)->debug("GetPositions::GetPositions: reading pos_info");
    unpack(result, payload, len, offset);
    std::pair<short, position_t> pos_info = result->as<std::pair<short, position_t>>();
    position_requests_[key] = PositionInfo(pos_info.first, pos_info.second);
  }

  // Parse matching data.
  spdlog::get(LOGGER)->debug("GetPositions::GetPositions: reading data");
  if (return_cmd_ == "select_synapse")
    data_ = std::make_shared<SelectSynapse>(payload, len, offset);
  else if (return_cmd_ == "set_wps")
    data_ = std::make_shared<SetWayPoints>(payload, len, offset);
  else if (return_cmd_ == "set_target")
    data_ = std::make_shared<SetTarget>(payload, len, offset);
}

// getter
std::string GetPositions::return_cmd() { return return_cmd_; }
std::map<short, Data::PositionInfo> GetPositions::position_requests() { return position_requests_; }
std::shared_ptr<Data> GetPositions::data() { return data_; }

// methods
void GetPositions::binary(std::stringstream& buffer) {
  msgpack::pack(buffer, return_cmd_);
  // Write number of position-requests
  msgpack::pack(buffer, position_requests_.size());
  // Write position-requests
  for (const auto& it : position_requests_) {
    // Write key
    msgpack::pack(buffer, it.first);
    // Convert and write value.
    std::pair<short, position_t> pos_info = {it.second._unit, it.second._pos};
    msgpack::pack(buffer, pos_info);
  }
  data_->binary(buffer);
}

// CHECK SEND AUDIO
CheckSendAudio::CheckSendAudio(std::string map_path) : Data(), same_device_(true), map_path_(map_path), 
  audio_file_name_("") {}
CheckSendAudio::CheckSendAudio(std::string map_path, std::string audio_file_name) : Data(), same_device_(false), 
  map_path_(map_path), audio_file_name_(audio_file_name) {}
CheckSendAudio::CheckSendAudio(const char* payload, size_t len, size_t& offset) : Data() {
  msgpack::object_handle result;

  unpack(result, payload, len, offset);
  same_device_ = result->as<bool>();
  unpack(result, payload, len, offset);
  map_path_ = result->as<std::string>();
  unpack(result, payload, len, offset);
  audio_file_name_ = result->as<std::string>();
}

// getter 
bool CheckSendAudio::same_device() { return same_device_; }
std::string CheckSendAudio::map_path() { return map_path_; }
std::string CheckSendAudio::audio_file_name() { return audio_file_name_; }

// methods
void CheckSendAudio::binary(std::stringstream& buffer) {
  msgpack::pack(buffer, same_device_);
  msgpack::pack(buffer, map_path_);
  msgpack::pack(buffer, audio_file_name_);
}

// SEND AUDIO INFO
SendAudioInfo::SendAudioInfo(bool send_audio, bool send_ai_audios) 
  : Data(), send_audio_(send_audio), send_ai_audios_(send_ai_audios) {}
SendAudioInfo::SendAudioInfo(const char* payload, size_t len, size_t& offset) : Data() {
  msgpack::object_handle result;

  unpack(result, payload, len, offset);
  send_audio_ = result->as<bool>();
  unpack(result, payload, len, offset);
  send_ai_audios_ = result->as<bool>();
}

// getter
bool SendAudioInfo::send_audio() { return send_audio_; }
bool SendAudioInfo::send_ai_audios() { return send_ai_audios_; }

// methods
void SendAudioInfo::binary(std::stringstream& buffer) {
  msgpack::pack(buffer, send_audio_);
  msgpack::pack(buffer, send_ai_audios_);
}

// AUDIO TRANSFER DATA
AudioTransferData::AudioTransferData(std::string username, std::string audioname) : Data() {
  username_ = username;
  audioname_ = audioname;
  part_ = 0;
  parts_ = 0;
  content_ = "";
  std::cout << "AudioTransferData successfully created" << std::endl;
}

AudioTransferData::AudioTransferData(const char* payload, size_t len, size_t& offset) : Data() {
  msgpack::object_handle result;

  unpack(result, payload, len, offset);
  username_ = result->as<std::string>();
  unpack(result, payload, len, offset);
  audioname_ = result->as<std::string>();
  unpack(result, payload, len, offset);
  part_ = result->as<int>();
  unpack(result, payload, len, offset);
  parts_ = result->as<int>();
  unpack(result, payload, len, offset);
  content_ = result->as<std::string>();
}

// getter
std::string AudioTransferData::username() { return username_; }
std::string AudioTransferData::songname() { return audioname_; }
std::string AudioTransferData::content() { return content_; }
int AudioTransferData::part() { return part_; } 
int AudioTransferData::parts() { return parts_; }

// setter 
void AudioTransferData::set_content(std::string content) { content_ = content; }
void AudioTransferData::set_parts(int parts) { parts_ = parts; }
void AudioTransferData::set_part(int part) { part_ = part; }

// Methods 
void AudioTransferData::binary(std::stringstream& buffer) {
  msgpack::pack(buffer, username_);
  msgpack::pack(buffer, audioname_);
  msgpack::pack(buffer, part_);
  msgpack::pack(buffer, parts_);
  msgpack::pack(buffer, content_);
}

// INITIALIZE GAME
InitializeGame::InitializeGame(std::string map_name) : Data(), map_name_(map_name) {}
InitializeGame::InitializeGame(const char* payload, size_t len, size_t& offset) {
  msgpack::object_handle result;

  unpack(result, payload, len, offset);
  map_name_ = result->as<std::string>();

  // read analysed_data and convert to jsons.
  std::map<std::string, std::string> analysed_data;
  unpack(result, payload, len, offset);
  analysed_data = result->as<std::map<std::string, std::string>>();
  for (const auto& it : analysed_data)
    ai_audio_data_[it.first] = nlohmann::json::parse(it.second);
}

// getter 
std::string InitializeGame::map_name() { return map_name_; }
std::map<std::string, nlohmann::json> InitializeGame::ai_audio_data() { return ai_audio_data_; }

// methods
void InitializeGame::binary(std::stringstream& buffer) {
  msgpack::pack(buffer, map_name_);
  // Convert jsons to string and then write.
  std::map<std::string, std::string> analysed_data;
  for (const auto& it : ai_audio_data_)
    analysed_data[it.first] = it.second.dump();
  msgpack::pack(buffer, analysed_data);
}

void InitializeGame::AddAiAudioData(std::string source_path, nlohmann::json analysed_data) {
  ai_audio_data_[source_path] = analysed_data;
}

// DISTRIBUTE IRON
DistributeIron::DistributeIron(unsigned short resource) : Data(), resource_(resource) {}
DistributeIron::DistributeIron(const char* payload, size_t len, size_t& offset) : Data() {
  msgpack::object_handle result;

  unpack(result, payload, len, offset);
  resource_ = result->as<unsigned short>();
}

// getter
unsigned short DistributeIron::resource() const { return resource_; }

// methods
void DistributeIron::binary(std::stringstream& buffer) {
  msgpack::pack(buffer, resource_);
}

// ADD TECHNOLOGY
AddTechnology::AddTechnology(unsigned short technology) : Data(), technology_(technology) {}
AddTechnology::AddTechnology(const char* payload, size_t len, size_t& offset) : Data() {
  msgpack::object_handle result;

  unpack(result, payload, len, offset);
  technology_ = result->as<unsigned short>();
}

// getter
unsigned short AddTechnology::technology() const { return technology_; }

// methods
void AddTechnology::binary(std::stringstream& buffer) {
  msgpack::pack(buffer, technology_);
}
