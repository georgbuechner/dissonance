#include "share/shemes/data.h"
#include "share/constants/codes.h"
#include "share/defines.h"
#include "share/tools/utils/utils.h"
#include <cstddef>
#include <memory>
#include <msgpack.hpp>
#include <string>
#include "nlohmann/json.hpp"

// SYMBOL 
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

// UPDATE_GAME
Update::Update(std::map<std::string, std::pair<std::string, int>> players, std::map<position_t, int> new_dead_neurons,
  float audio_played) : Data(), players_(players), new_dead_neurons_(new_dead_neurons), audio_played_(audio_played) {};

Update::Update(const char* payload, size_t len, size_t& offset) : Data() {
  msgpack::object_handle result;

  unpack(result, payload, len, offset);
  players_ = result->as<std::map<std::string, std::pair<std::string, int>>>();

  unpack(result, payload, len, offset);
  new_dead_neurons_ = result->as<std::map<position_t, int>>();

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
  msgpack::pack(buffer, new_dead_neurons_);
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

  // Create update-object
  update_ = std::make_shared<Update>(payload, len, offset);
}

// getter 
std::vector<std::vector<Data::Symbol>> Init::field() { return field_; }
std::vector<position_t> Init::graph_positions() { return graph_positions_; }
std::map<int, tech_of_t> Init::technologies() { return technologies_; }
std::shared_ptr<Update> Init::update() { return update_; }
short Init::macro() { return macro_; }

// setter 
void Init::set_macro(short macro) { macro_ = macro; }

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
  update_->binary(buffer);
}


// SET_UNIT 
FieldPosition::FieldPosition(position_t pos, short unit, short color) : Data(), pos_(pos), unit_(unit), color_(color) {}
FieldPosition::FieldPosition(const char* payload, size_t len, size_t& offset) : Data() {
  msgpack::object_handle result;

  unpack(result, payload, len, offset);
  pos_ = result->as<position_t>();

  unpack(result, payload, len, offset);
  unit_ = result->as<short>();

  unpack(result, payload, len, offset);
  color_ = result->as<short>();
}

// getter 
position_t FieldPosition::pos() { return pos_; }
short FieldPosition::unit() { return unit_; }
short FieldPosition::color() { return color_; }

// methods 
void FieldPosition::binary(std::stringstream& buffer) {
  msgpack::pack(buffer, pos_);
  msgpack::pack(buffer, unit_);
  msgpack::pack(buffer, color_);
}

// SET_UNITS

Units::Units(std::vector<FieldPosition> units) : Data(), units_(units) {}
Units::Units(const char* payload, size_t len, size_t& offset) : Data() {
  while (offset != len) {
    units_.push_back(FieldPosition(payload, len, offset));
  }
}

// getter 
std::vector<FieldPosition> Units::units() { return units_; }

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
}

// getter
short Statictics::player_color() const { return player_color_; }
std::map<unsigned short, unsigned short> Statictics::neurons_build() const { return neurons_build_; }
std::map<unsigned short, unsigned short> Statictics::potentials_build() const { return potentials_build_; }
std::map<unsigned short, unsigned short> Statictics::potentials_killed() const { return potentials_killed_; }
std::map<unsigned short, unsigned short> Statictics::potentials_lost() const { return potentials_lost_; }
unsigned short Statictics::epsp_swallowed() const { return epsp_swallowed_; }
std::map<int, std::map<std::string, double>> Statictics::stats_resources() const { return resources_; }
std::map<int, tech_of_t> Statictics::stats_technologies() const { return technologies_; }
std::map<int, std::map<std::string, double>>& Statictics::stats_resources_ref() { return resources_; }

// setter
void Statictics::set_color(short color) { player_color_ = color; }
void Statictics::set_technologies(std::map<int, tech_of_t> technologies) { technologies_ = technologies; }

// methods
void Statictics::AddNewNeuron(int unit) {
  neurons_build_[unit]++;
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

void Statictics::print() {
  std::cout << "Neurons built: " << std::endl;
  for (const auto& it : neurons_build_) 
    std::cout << " - " << units_tech_name_mapping.at(it.first) << ": " << it.second << std::endl;
  std::cout << "Potentials built: " << std::endl;
  for (const auto& it : potentials_build_) 
    std::cout << " - " << units_tech_name_mapping.at(it.first) << ": " << it.second << std::endl;
  std::cout << "Potentials killed: " << std::endl;
  for (const auto& it : potentials_killed_) 
    std::cout << " - " << units_tech_name_mapping.at(it.first) << ": " << it.second << std::endl;
  std::cout << "Potentials lost: " << std::endl;
  for (const auto& it : potentials_lost_) 
    std::cout << " - " << units_tech_name_mapping.at(it.first) << ": " << it.second << std::endl;
  std::cout << "Enemy epsps swallowed by ipsp: " << epsp_swallowed_ << std::endl;
}

void Statictics::binary(std::stringstream& buffer) {
  msgpack::pack(buffer, player_color_);
  msgpack::pack(buffer, neurons_build_);
  msgpack::pack(buffer, potentials_build_);
  msgpack::pack(buffer, potentials_killed_);
  msgpack::pack(buffer, potentials_lost_);
  msgpack::pack(buffer, epsp_swallowed_);
  msgpack::pack(buffer, resources_);
  msgpack::pack(buffer, technologies_);
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
BuildPotential::BuildPotential(short unit, short num) : Data(), unit_(unit), num_(num), start_pos_({-1, -1}) {}
BuildPotential::BuildPotential(const char* payload, size_t len, size_t& offset) : Data() {
  msgpack::object_handle result;
  unpack(result, payload, len, offset);
  unit_ = result->as<short>();
  unpack(result, payload, len, offset);
  num_ = result->as<short>();
  unpack(result, payload, len, offset);
  start_pos_ = result->as<position_t>();
  unpack(result, payload, len, offset);
  positions_ = result->as<std::vector<position_t>>();
}

// getter 
short BuildPotential::unit() { return unit_; }
short BuildPotential::num() { return num_; }
position_t BuildPotential::start_pos() { return start_pos_; }
std::vector<position_t> BuildPotential::positions() { return positions_; }

// setter 
void BuildPotential::set_start_pos(position_t pos) { start_pos_ = pos; }
void BuildPotential::set_positions(std::vector<position_t> positions) { positions_ = positions; }

// methods
void BuildPotential::binary(std::stringstream& buffer) {
  msgpack::pack(buffer, unit_);
  msgpack::pack(buffer, num_);
  msgpack::pack(buffer, start_pos_);
  msgpack::pack(buffer, positions_);
}

void BuildPotential::SetPickedPosition(position_t pos) { start_pos_ = pos; }

// BUILD NEURON
BuildNeuron::BuildNeuron(short unit) : Data(), unit_(unit), range_(0), pos_({-1, -1}), start_pos_({-1, -1}) {}
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
short BuildNeuron::unit() { return unit_; }
position_t BuildNeuron::pos() { return pos_; }
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

void BuildNeuron::SetPickedPosition(position_t pos) { start_pos_ = pos; }

// SELECT SYNAPSE
SelectSynapse::SelectSynapse(std::vector<position_t> positions) : Data(), synapse_pos_({-1, -1}), positions_(positions) {}
SelectSynapse::SelectSynapse(const char* payload, size_t len, size_t& offset) : Data() {
  msgpack::object_handle result;
  unpack(result, payload, len, offset);
  synapse_pos_ = result->as<position_t>();
  unpack(result, payload, len, offset);
  positions_ = result->as<std::vector<position_t>>();
}

// getter 
position_t SelectSynapse::synapse_pos() { return synapse_pos_; }
std::vector<position_t> SelectSynapse::positions() { return positions_; }

// setter 
void SelectSynapse::set_synapse_pos(position_t pos) { synapse_pos_ = pos; }

// methods
void SelectSynapse::binary(std::stringstream& buffer) { 
  msgpack::pack(buffer, synapse_pos_);
  msgpack::pack(buffer, positions_);
}

/**
 * Used by Pick-Context to set selected ('picked') position. 
 * Function needs to exists for all data-structs dependant on Pick-Context.
 */
void SelectSynapse::SetPickedPosition(position_t pos) { synapse_pos_ = pos; }


// SET WAYPOINTS
SetWayPoints::SetWayPoints(position_t synapse_pos) : Data(), synapse_pos_(synapse_pos) {
  way_point_ = {-1, -1};
  msg_ = "";
  num_ = 1;
};

SetWayPoints::SetWayPoints(const char* payload, size_t len, size_t& offset) : Data() {
  msgpack::object_handle result;

  unpack(result, payload, len, offset);
  synapse_pos_ = result->as<position_t>();
  unpack(result, payload, len, offset);
  way_point_ = result->as<position_t>();
  unpack(result, payload, len, offset);
  positions_ = result->as<std::vector<position_t>>();
  unpack(result, payload, len, offset);
  msg_ = result->as<std::string>();
  unpack(result, payload, len, offset);
  num_ = result->as<short>();
}

// getter 
position_t SetWayPoints::synapse_pos() { return synapse_pos_; }
position_t SetWayPoints::way_point() { return way_point_; }
std::vector<position_t> SetWayPoints::positions() { return positions_; }
std::string SetWayPoints::msg() { return msg_; }
short SetWayPoints::num() { return num_; }

// setter 
void SetWayPoints::set_way_point(position_t pos) { way_point_ = pos; }
void SetWayPoints::set_positions(std::vector<position_t> positions) { positions_ = positions; }
void SetWayPoints::set_msg(std::string msg) { msg_ = msg; }
void SetWayPoints::set_num(short num) { num_ = num; }

// methods
void SetWayPoints::binary(std::stringstream& buffer) {
  msgpack::pack(buffer, synapse_pos_);
  msgpack::pack(buffer, way_point_);
  msgpack::pack(buffer, positions_);
  msgpack::pack(buffer, msg_);
  msgpack::pack(buffer, num_);
}

void SetWayPoints::SetPickedPosition(position_t pos) { way_point_ = pos; }

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
  positions_ = result->as<std::vector<position_t>>();
}

// getter 
short SetTarget::unit() { return unit_; }
position_t SetTarget::synapse_pos() { return synapse_pos_; }
position_t SetTarget::start_pos() { return start_pos_; }
position_t SetTarget::target() { return target_; }
std::vector<position_t> SetTarget::positions() { return positions_; }

// setter 
void SetTarget::set_start_pos(position_t pos) { start_pos_ = pos; }
void SetTarget::set_target(position_t pos) { target_ = pos; }
void SetTarget::set_positions(std::vector<position_t> positions) { positions_ = positions; }

// methods
void SetTarget::binary(std::stringstream& buffer) {
  msgpack::pack(buffer, unit_);
  msgpack::pack(buffer, synapse_pos_);
  msgpack::pack(buffer, start_pos_);
  msgpack::pack(buffer, target_);
  msgpack::pack(buffer, positions_);
}

void SetTarget::SetPickedPosition(position_t pos) { start_pos_ = pos; }

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

  unpack(result, payload, len, offset);
  return_cmd_ = result->as<std::string>();

  // Parse position-requests
  unpack(result, payload, len, offset);
  unsigned int num_requests = result->as<unsigned int>();
  for (unsigned int i=0; i<num_requests; i++) {
    unpack(result, payload, len, offset);
    short key = result->as<short>();
    unpack(result, payload, len, offset);
    std::pair<short, position_t> pos_info = result->as<std::pair<short, position_t>>();
    position_requests_[key] = PositionInfo(pos_info.first, pos_info.second);
  }

  // Parse matching data.
  if (return_cmd_ == "select_synapse")
    data_ = std::make_shared<SelectSynapse>(payload, len, offset);
  else if (return_cmd_ == "set_wps")
    data_ = std::make_shared<SetWayPoints>(payload, len, offset);
  else if (return_cmd_ == "target")
    data_ = std::make_shared<SetTarget>(payload, len, offset);
}

// getter
std::string GetPositions::return_cmd() { return return_cmd_; }
std::map<short, Data::PositionInfo> GetPositions::position_requests() { return position_requests_; }
std::shared_ptr<Data> GetPositions::data() { return data_; }

// methods
void GetPositions::binary(std::stringstream& buffer) {
  msgpack::pack(buffer, return_cmd_);
  msgpack::pack(buffer, position_requests_.size());
  for (const auto& it : position_requests_) {
    msgpack::pack(buffer, it.first);
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

// AUDIO TRANSFER DATA
AudioTransferDataNew::AudioTransferDataNew(std::string username, std::string audioname) : Data() {
  username_ = username;
  audioname_ = audioname;
  part_ = 0;
  parts_ = 0;
  content_ = "";
  std::cout << "AudioTransferData successfully created" << std::endl;
}

AudioTransferDataNew::AudioTransferDataNew(const char* payload, size_t len, size_t& offset) : Data() {
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
std::string AudioTransferDataNew::username() { return username_; }
std::string AudioTransferDataNew::songname() { return audioname_; }
std::string AudioTransferDataNew::content() { return content_; }
int AudioTransferDataNew::part() { return part_; } 
int AudioTransferDataNew::parts() { return parts_; }

// setter 
void AudioTransferDataNew::set_content(std::string content) { content_ = content; }
void AudioTransferDataNew::set_parts(int parts) { parts_ = parts; }
void AudioTransferDataNew::set_part(int part) { part_ = part; }

// Methods 
void AudioTransferDataNew::binary(std::stringstream& buffer) {
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
unsigned short DistributeIron::resource() { return resource_; }

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
unsigned short AddTechnology::technology() { return technology_; }

// methods
void AddTechnology::binary(std::stringstream& buffer) {
  msgpack::pack(buffer, technology_);
}
