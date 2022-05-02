#include "server/game/server_game.h"
#include "nlohmann/json_fwd.hpp"
#include "server/game/player/audio_ki.h"
#include "server/game/player/player.h"
#include "share/audio/audio.h"
#include "share/constants/codes.h"
#include "share/defines.h"
#include "share/objects/dtos.h"
#include "share/objects/transfer.h"
#include "share/objects/units.h"
#include "server/websocket/websocket_server.h"
#include "share/tools/eventmanager.h"
#include "share/tools/random/random.h"
#include "share/tools/utils/utils.h"

#include "spdlog/spdlog.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>

bool IsAi(std::string username) {
  return username.find("AI") != std::string::npos;
}

ServerGame::ServerGame(int lines, int cols, int mode, int num_players, std::string base_path, 
    WebsocketServer* srv) : max_players_(num_players), cur_players_(1), audio_(base_path), ws_server_(srv), 
    status_(WAITING), mode_((mode == TUTORIAL) ? SINGLE_PLAYER : mode), lines_(lines), cols_(cols) {
  pause_ = false;
  time_in_pause_ = 0;
  tutorial_ = mode == TUTORIAL;
  audio_data_ = "";
  audio_file_name_ = "";
  base_path_ = base_path;
  // Initialize eventmanager.
  eventmanager_.AddHandler("audio_map", &ServerGame::m_SendAudioMap);
  eventmanager_.AddHandler("initialize_game", &ServerGame::m_InitializeGame);
  eventmanager_.AddHandler("add_iron", &ServerGame::m_AddIron);
  eventmanager_.AddHandler("remove_iron", &ServerGame::m_RemoveIron);
  eventmanager_.AddHandler("add_technology", &ServerGame::m_AddTechnology);
  eventmanager_.AddHandler("resign", &ServerGame::m_Resign);
  eventmanager_.AddHandler("check_build_neuron", &ServerGame::m_CheckBuildNeuron);
  eventmanager_.AddHandler("check_build_potential", &ServerGame::m_CheckBuildPotential);
  eventmanager_.AddHandler("build_neuron", &ServerGame::m_BuildNeurons);
  eventmanager_.AddHandler("get_positions", &ServerGame::m_GetPositions);
  eventmanager_.AddHandler("toggle_swarm_attack", &ServerGame::m_ToggleSwarmAttack);
  eventmanager_.AddHandler("set_way_point", &ServerGame::m_SetWayPoint);
  eventmanager_.AddHandler("set_target", &ServerGame::m_SetTarget);
  eventmanager_.AddHandler("set_pause_on", &ServerGame::m_SetPauseOn);
  eventmanager_.AddHandler("set_pause_off", &ServerGame::m_SetPauseOff);
}

// getter

int ServerGame::status() {
  return status_;
}

int ServerGame::mode() {
  return mode_;
}
int ServerGame::max_players() {
  return max_players_;
}
int ServerGame::cur_players() {
  return cur_players_;
}
std::string ServerGame::audio_map_name() {
  return audio_.filename(false);
}


// setter 
void ServerGame::set_status(int status) {
  std::unique_lock ul(mutex_status_);
  status_ = status;
}

void ServerGame::PrintStatistics() {
  for (const auto& it : players_) {
    std::string status = (it.second->HasLost()) ? "LOST" : "WON";
    std::cout << it.first << " (" << status << " " << it.second->GetNucleusLive() << ")" << std::endl;
    it.second->statistics().print();
  }
}

void ServerGame::AddPlayer(std::string username, int lines, int cols) {
  std::unique_lock ul(mutex_players_);
  spdlog::get(LOGGER)->info("ServerGame::AddPlayer: {}", username);
  // Check is free slots in lobby.
  if (players_.size() < max_players_) {
    players_[username] = nullptr;
    human_players_[username] = nullptr;
    // Adjust field size and width
    lines_ = (lines < lines_) ? lines : lines_;
    cols_ = (cols < cols_) ? cols : cols_;
    // Send audio-data to new player.
    SendSong(username);
  }
}

void ServerGame::SendSong(std::string username) {
  // Create initial data
  AudioTransferData data(host_, audio_file_name_);
  std::map<int, std::string> contents;
  utils::SplitLargeData(contents, audio_data_, pow(2, 12));
  spdlog::get(LOGGER)->info("Made {} parts of {} bits data", contents.size(), audio_data_.size());
  data.set_parts(contents.size()-1);
  for (const auto& it : contents) {
    data.set_part(it.first);
    data.set_content(it.second);
    try {
      ws_server_->SendMessageBinary(username, data.string());
    } catch(...) {
      return;
    }
  }
  return;
}

void ServerGame::AddAudioPart(AudioTransferData& data) {
  // Set song name if not already set.
  if (audio_file_name_ == "")
    audio_file_name_ = data.songname();
  std::string path = base_path_ + "data/user-files/"+data.username();
  // Create directory for this user.
  if (!std::filesystem::exists(path))
    std::filesystem::create_directory(path);
  path += "/"+data.songname();
  audio_data_+=data.content();
  if (data.part() == data.parts()) {
    spdlog::get(LOGGER)->debug("Websocket::on_message: storing binary data to: {}", path);
    utils::StoreMedia(path, audio_data_);
  }
}

void ServerGame::PlayerReady(std::string username) {
  // Only start game if status is still waiting, to avoid starting game twice.
  if (players_.size() >= max_players_ && status_ == WAITING_FOR_PLAYERS) {
    spdlog::get(LOGGER)->info("ServerGame::AddPlayer: starting game as last player entered game.");
    StartGame({});
  }
}

nlohmann::json ServerGame::HandleInput(std::string command, nlohmann::json msg) {
  // Check if command is known.
  if (eventmanager_.handlers().count(command)) {
    // Lock mutex! And do player action.
    std::unique_lock ul(mutex_players_);
    (this->*eventmanager_.handlers().at(command))(msg);
  }
  else 
    msg = nlohmann::json();
  spdlog::get(LOGGER)->debug("ClientGame::HandleAction: response {}", msg.dump());
  return msg;
}

// command methods

void ServerGame::m_AddIron(nlohmann::json& msg) {
  // Get username player and user.
  std::string username = msg["username"];
  Player* player = (players_.count(username) > 0) ? players_.at(username) : NULL;
  int resource = msg["data"]["resource"];
  if (player && player->DistributeIron(resource)) {
    msg = {{"command", "set_msg"}, {"data", {{"msg", "Distribute iron: done!"}} }};
    // If resource is newly created, send client resource-neuron as new unit.
    if (player->resources().at(resource).distributed_iron() == 2) {
      nlohmann::json req = {{"command", "set_unit"}, {"data", {{"unit", RESOURCENEURON}, 
        {"pos", player->resources().at(resource).pos()}, {"color", COLOR_RESOURCES}, {"resource", resource}} }};
      ws_server_->SendMessage(username, req);
    }
  }
  else 
    msg = {{"command", "set_msg"}, {"data", {{"msg", "Distribute iron: not enough iron!"}} }};
}

void ServerGame::m_RemoveIron(nlohmann::json& msg) {
  Player* player = (players_.count(msg["username"]) > 0) ? players_.at(msg["username"]) : NULL;
  int resource = msg["data"]["resource"];
  if (player && player->RemoveIron(msg["data"]["resource"])) {
    msg = {{"command", "set_msg"}, {"data", {{"msg", "Remove iron: done!"}} }};
    // If resource is now below produce-minimum, send client resource-neuron as new unit.
    if (player->resources().at(resource).bound() == 1) {
      nlohmann::json req = {{"command", "set_unit"}, {"data", {{"unit", RESOURCENEURON}, 
        {"pos", player->resources().at(resource).pos()}, {"color", COLOR_DEFAULT}} }};
      ws_server_->SendMessage(msg["username"], req);
    }
  }
  else 
    msg = {{"command", "set_msg"}, {"data", {{"msg", "Remove iron: not enough iron!"}} }};
}

void ServerGame::m_AddTechnology(nlohmann::json& msg) {
  Player* player = (players_.count(msg["username"]) > 0) ? players_.at(msg["username"]) : NULL;
  // If player exists, add technology and send mathcing response.
  if (player && player->AddTechnology(msg["data"]["technology"]))
    msg = {{"command", "set_msg"}, {"data", {{"msg", "Add technology: done!"}} }};
  else 
    msg = {{"command", "set_msg"}, {"data", {{"msg", "Add technology: probably not enough resources!"}} }};
}

void ServerGame::m_Resign(nlohmann::json& msg) {
  std::unique_lock ul(mutex_status_);
  status_ = CLOSING;
  ul.unlock();
  // If multi player, inform other player.
  std::map<std::string, nlohmann::json> statistics;
  for (const auto& it : players_) 
    statistics[it.first] = it.second->GetFinalStatistics();
  nlohmann::json resp = {{"command", "game_end"}, {"data", {{"msg", "YOU WON - opponent resigned"}, 
    {"statistics", statistics}} }};
  SendMessageToAllPlayers(resp, msg["username"]);
  resp = {{"command", "game_end"}, {"data", {{"msg", "YOU RESIGNED"}, {"statistics", statistics}} }};
  ws_server_->SendMessage(msg["username"], resp);
  msg = nlohmann::json();
}

void ServerGame::m_CheckBuildNeuron(nlohmann::json& msg) {
  int unit = msg["data"]["unit"];
  Player* player = (players_.count(msg["username"]) > 0) ? players_.at(msg["username"]) : NULL;
  if (player) {
    std::string missing = GetMissingResourceStr(player->GetMissingResources(unit));
    std::vector<position_t> positions;
    if (unit == NUCLEUS) 
      positions = field_->GetAllCenterPositionsOfSections();
    else 
      positions = player->GetAllPositionsOfNeurons(NUCLEUS);
    // Can be build (enough resources) and start-position for selecting pos is known (only one nucleus)
    if (missing == "" && positions.size() == 1)
      msg = {{"command", "build_neuron"}, {"data", {{"unit", unit}, {"start_pos", positions.front()}, 
        {"range", player->cur_range()}} }}; 
    // Can be build (enough resources) and start-position for selecting pos is unknown (send positions)
    else if (missing == "") 
      msg = {{"command", "build_neuron"}, {"data", {{"unit", unit}, {"positions", positions}, 
        {"range", player->cur_range()}} }}; 
    // Otherwise send error (not-enough-resource) message.
    else 
      msg = {{"command", "set_msg"}, {"data", {{"msg", "Not enough resource! missing: " + missing}}}};
  }
  else {
    msg = nlohmann::json();
  }
}

void ServerGame::m_CheckBuildPotential(nlohmann::json& msg) {
  int unit = msg["data"].value("unit", -1);
  int num = msg["data"].value("num", -1);
  if (num == -1 || unit == -1) {
    msg = nlohmann::json();
    return;
  }
  Player* player = (players_.count(msg["username"]) > 0) ? players_.at(msg["username"]) : NULL;
  if (player) {
    auto synapses = player->GetAllPositionsOfNeurons(SYNAPSE);
    std::string missing = GetMissingResourceStr(player->GetMissingResources(unit));
    // Missing resources => error message
    if (missing != "")
      msg = {{"command", "set_msg"}, {"data", {{"msg", "Not enough resource! missing: " + missing}}}};
    // No synapses => error message
    else if (synapses.size() == 0)
      msg = {{"command", "set_msg"}, {"data", {{"msg", "No synapse!"}} }};
    // Only one synapse or player specified position => add potential
    else if (synapses.size() == 1 || msg["data"].contains("start_pos")) {
      position_t pos = (synapses.size() == 1) ? synapses.front() : utils::PositionFromVector(msg["data"]["start_pos"]);
      BuildPotentials(unit, pos, num, msg["username"], player);
      msg = nlohmann::json();
    }
    // More than one synapse and position not given => tell user to select position.
    else  {
      msg = {{"command", "build_potential"}, {"data", {{"unit", unit}, {"positions", 
        player->GetAllPositionsOfNeurons(SYNAPSE)}, {"num", num}} }}; 
    }
  }
  else {
    msg = nlohmann::json();
  }
}

void ServerGame::m_BuildNeurons(nlohmann::json& msg) {
  int unit = msg["data"]["unit"];
  position_t pos = {msg["data"]["pos"][0], msg["data"]["pos"][1]};
  Player* player = (players_.count(msg["username"]) > 0) ? players_.at(msg["username"]) : NULL;
  if (player) {
    bool success = false;
    // In case of synapse get random position for epsp- and ipsp-target.
    if (unit == SYNAPSE)
      success = player->AddNeuron(pos, unit, player->enemies().front()->GetOneNucleus(), 
        player->enemies().front()->GetRandomNeuron());
    // Otherwise simply add.
    else 
      success = player->AddNeuron(pos, unit);
    // Add position to field, tell all players to add position and send success mesage.
    if (success) {
      field_->AddNewUnitToPos(pos, unit);
      msg = {{"command", "set_unit"}, {"data", {{"unit", unit}, {"pos", pos}, 
        {"color", player->color()}} }};
    }
    else
      msg = {{"command", "set_msg"}, {"data", {{"msg", "Failed!"}} }};
  }
  else {
    msg = nlohmann::json();
  }
}

void ServerGame::m_GetPositions(nlohmann::json& msg) {
  Player* player = (players_.count(msg["username"]) > 0) ? players_.at(msg["username"]) : NULL;
  GetPosition req = GetPosition(msg);
  if (player) {
    msg = {{"command", msg["data"]["return_cmd"]}, {"data", nlohmann::json() }};
    std::vector<std::vector<position_t>> all_positions;
    for (const auto& it : req.position_requests()) {
      std::vector<position_t> positions;
      // player-units
      if (it.first == Positions::PLAYER)
        positions = player->GetAllPositionsOfNeurons(it.second.unit());
      // enemy-units
      else if (it.first == Positions::ENEMY) {
        for (const auto& enemy : player->enemies()) 
          for (const auto& it : enemy->GetAllPositionsOfNeurons(it.second.unit()))
            positions.push_back(it);
      }
      // center-positions
      else if (it.first == Positions::CENTER)
        positions = field_->GetAllCenterPositionsOfSections();
      // ipsp-/ epsp-targets
      else if (it.first == Positions::TARGETS) {
        position_t ipsp_target_pos = player->GetSynapesTarget(it.second.pos(), it.second.unit());
        if (ipsp_target_pos.first != -1)
          positions.push_back(ipsp_target_pos);
        msg["data"]["unit"] = it.second.unit();
      }
      else if (it.first == Positions::CURRENT_WAY) {
        // Get way to ipsp-target
        for (const auto& it : field_->GetWayForSoldier(it.second.pos(), 
              player->GetSynapesWayPoints(it.second.pos(), IPSP)))
          positions.push_back(it);
        // Get way to epsp-target
        for (const auto& it : field_->GetWayForSoldier(it.second.pos(), 
              player->GetSynapesWayPoints(it.second.pos(), EPSP)))
          positions.push_back(it);
      }
      else if (it.first == Positions::CURRENT_WAY_POINTS) {
        positions = player->GetSynapesWayPoints(it.second.pos());
      }
      all_positions.push_back(positions);
    }
    msg["data"]["positions"] = all_positions;
  }
  else {
    msg = nlohmann::json();
  }
}

void ServerGame::m_ToggleSwarmAttack(nlohmann::json& msg) {
  Player* player = (players_.count(msg["username"]) > 0) ? players_.at(msg["username"]) : NULL;
  if (player) {
    // Toggle swarm attack and get wether now active/ inactive.
    std::string on_off = (player->SwitchSwarmAttack(utils::PositionFromVector(msg["data"]["pos"]))) ? "on" : "off";
    msg = {{"command", "set_msg"}, {"data", {{"msg", "Toggle swarm-attack successfull. Swarm attack " + on_off}} }};
  }
  else {
    msg = nlohmann::json();
  }
}

void ServerGame::m_SetWayPoint(nlohmann::json& msg) {
  Player* player = (players_.count(msg["username"]) > 0) ? players_.at(msg["username"]) : NULL;
  if (player) {
    int num = msg["data"]["num"]; // number of way-point added.
    int tech = player->technologies().at(WAY).first; // number of availible way points to add.
    position_t synapse_pos = utils::PositionFromVector(msg["data"]["synapse_pos"]);
    // If first way-point, reset waypoints with new way-point.
    if (num == 1) 
      player->ResetWayForSynapse(synapse_pos, msg["data"]["pos"]);
    // Otherwise add waypoint.
    else 
      player->AddWayPosForSynapse(synapse_pos, msg["data"]["pos"]);
    // Send response to player with telling player how many waypoints have now been set.
    std::string x_of = std::to_string(num) + "/" + std::to_string(tech);
    msg = {{"command", "set_wps"}, {"data", {{"msg", "New way-point added: " + x_of}, {"synapse_pos", synapse_pos}} }};
    // If there are waypoints left to set, send next way-point-number (-1 otherwise)
    msg["data"]["num"] = (num < tech) ? num+1 : -1;
  }
  else {
    msg = nlohmann::json();
  }
}

void ServerGame::m_SetTarget(nlohmann::json& msg) {
  Player* player = (players_.count(msg["username"]) > 0) ? players_.at(msg["username"]) : NULL;
  if (player) {
    // Change ipsp target and response with message
    int unit = msg["data"]["unit"];
    if (unit == IPSP)
      player->ChangeIpspTargetForSynapse(msg["data"]["synapse_pos"], msg["data"]["pos"]);
    else if (unit == EPSP)
      player->ChangeEpspTargetForSynapse(msg["data"]["synapse_pos"], msg["data"]["pos"]);
    else if (unit == MACRO)
      player->ChangeMacroTargetForSynapse(msg["data"]["synapse_pos"], msg["data"]["pos"]);
    msg = {{"command", "set_msg"}, {"data", {{"msg", "Target for this synapse set"}} }};
  }
  else {
    msg = nlohmann::json();
  }
}

void ServerGame::m_SetPauseOn(nlohmann::json& msg) {
  spdlog::get(LOGGER)->info("Set pause on");
  pause_ = true; 
  pause_start_ = std::chrono::steady_clock::now();
  msg = nlohmann::json();
}

void ServerGame::m_SetPauseOff(nlohmann::json& msg) {
  pause_ = false;
  std::unique_lock ul(mutex_pause_);
  time_in_pause_ += utils::GetElapsed(pause_start_, std::chrono::steady_clock::now());
  spdlog::get(LOGGER)->info("Set pause off {}", time_in_pause_);
  msg = nlohmann::json();
}

void ServerGame::BuildPotentials(int unit, position_t pos, int num, std::string username, Player* player) {
  bool success = false;
  // (Try to) build number of given potentials to build.
  for (int i=0; i < num; i++) {
    success = player->AddPotential(pos, unit, i/2);
    if (!success)
      break;
  }
  // Create and send message indicating success/ failiure.
  nlohmann::json msg = {{"command", "set_msg"}, {"data", {{"msg", (success) ? "Success!" : "Failed!"}} }};
  ws_server_->SendMessage(username, msg);
}

void ServerGame::m_SendAudioMap(nlohmann::json& msg) {
  spdlog::get(LOGGER)->info("ServerGame::InitializeGame: initializing with user: {} {}", host_, mode_);
  host_ = msg["username"];
  bool send_song = false;
  if (msg["data"].contains("map_path_same_device")) {
    audio_.set_source_path(msg["data"]["map_path_same_device"]);
  }
  else {
    std::string path = base_path_ + "/data/user-files/" + msg["data"]["map_path"].get<std::string>();
    audio_.set_source_path(path);
    if (std::filesystem::exists(path)) {
      audio_data_ = utils::GetMedia(path);
      audio_file_name_ = msg["data"]["song_name"];
    }
    else 
      send_song = true;
  }
  msg["command"] = "send_audio_info";
  msg["data"] = {{"send_song", send_song}};
}

void ServerGame::m_InitializeGame(nlohmann::json& msg) {
  spdlog::get(LOGGER)->info("ServerGame::InitializeGame: initializing with user: {} {}", host_, mode_);
  nlohmann::json data = msg["data"];

  // Get and analyze main audio-file (used for map and in SP for AI).
  std::string map_name = data["map_name"];
  audio_.Analyze();
  if (map_name.size() > 10) 
    map_name = map_name.substr(0, 10);

  // Get and analyze audio-files for AIs (OBSERVER-mode).
  std::vector<Audio*> audios; 
  if (msg["data"].contains("ais")) {
    for (const auto& it : msg["data"]["ais"].get<std::map<std::string, nlohmann::json>>()) {
      Audio* new_audio = new Audio(base_path_);
      new_audio->set_source_path(it.first);
      // Analyze with given base-anaysis.
      new_audio->Analyze(it.second);
      audios.push_back(new_audio);
    }
  }

  // Add host to players.
  if (mode_ != OBSERVER) {
    players_[host_] = nullptr;
    human_players_[host_] = nullptr;
  }
  // Or to observers.
  else if (mode_ == OBSERVER)
    observers_.push_back(host_);

  // If SINGLE_PLAYER, add AI to players and start game.
  if (mode_ == SINGLE_PLAYER) {
    players_["AI (" + map_name + ")"] = nullptr;
    StartGame(audios);
  }
  // If observers, add both ais.
  else if (mode_ == OBSERVER) {
    players_["AI (" + audios[0]->filename(true) + ")"] = nullptr;
    players_["AI (" + audios[1]->filename(true) + ")"] = nullptr;
    StartGame(audios);
  }
  // Otherwise send info "waiting for players" to host.
  else {
    status_ = WAITING_FOR_PLAYERS;
    msg["command"] = "print_msg";
    msg["data"] = {{"msg", "Wainting for players..."}};
    ws_server_->SendMessage(host_, msg);
  }
  msg = nlohmann::json();
}

void ServerGame::PlayerResigned(std::string username) {
  if (players_.count(username) > 0 && players_.at(username) != nullptr && dead_players_.count(username) == 0) {
    players_.at(username)->set_lost(true);
    SendMessageToAllPlayers({{"command", "set_msg"}, {"data", {{"msg", username + " resigned"}}}}, username);
  }
}

void ServerGame::InitAiGame(std::string base_path, std::string path_audio_map, std::string path_audio_a, 
    std::string path_audio_b) {
  // Analyze map audio
  audio_.set_source_path(path_audio_map);
  audio_.Analyze();
  // Analyze audio for ais.
  std::vector<Audio*> audios; 
  // Analyze audio for first ai.
  Audio* audio_a = new Audio(base_path);
  audio_a->set_source_path(path_audio_a);
  audio_a->Analyze();
  audios.push_back(audio_a);
  // Analyze audio for second ai.
  Audio* audio_b = new Audio(base_path);
  audio_b->set_source_path(path_audio_b);
  audio_b->Analyze();
  audios.push_back(audio_b);

  // Create players
  players_["AI (" + audios[0]->filename(true) + ")"] = nullptr;
  players_["AI (" + audios[1]->filename(true) + ")"] = nullptr;

  // Initialize game (for ai-games `StartGame` does not start game, but only create map etc.)
  StartGame(audios);
  // Actually start AI game and keep track of time.
  auto audio_start_time = std::chrono::steady_clock::now();
  int one_player_won = RunAiGame();
  auto elapsed = utils::GetElapsed(audio_start_time, std::chrono::steady_clock::now());
  
  // Print a few metadatas
  std::cout << "Game took " << elapsed << " milli seconds " << std::endl;
  if (!one_player_won)
    std::cout << "BOTH AIS LOST (TIME WAS UP!)" << std::endl;
  PrintStatistics();
}

void ServerGame::StartGame(std::vector<Audio*> audios) {
  // Initialize field.
  RandomGenerator* ran_gen = new RandomGenerator(audio_.analysed_data(), &RandomGenerator::ran_note);
  auto nucleus_positions = SetUpField(ran_gen);
  if (nucleus_positions.size() < max_players_)
    return;

  // Setup players.
  std::string nucleus_positions_str = "nucleus': ";
  for (const auto& it : nucleus_positions)
    nucleus_positions_str += utils::PositionToString(it) + ", ";
  spdlog::get(LOGGER)->info("ServerGame::InitializeGame: Creating {} players at {}", max_players_, nucleus_positions_str);
  unsigned int counter = 0;
  unsigned int ai_counter = 0;
  for (const auto& it : players_) {
    int color = (counter % 4) + 10; // currently results in four different colors
    // Add AI
    if (IsAi(it.first)) {
      // If audios are given, uses one of the audios (observer- or ai-game)
      if (audios.size() == 0)
        players_[it.first] = new AudioKi(nucleus_positions[counter], field_, &audio_, ran_gen, color);
      // Other wise uses same audio as map.
      else 
        players_[it.first] = new AudioKi(nucleus_positions[counter], field_, audios[ai_counter++], ran_gen, color);
    }
    // Add human player
    else {
      players_[it.first] = new Player(nucleus_positions[counter], field_, ran_gen, color);
      human_players_[it.first] = players_[it.first];
    }
    counter++;
  }
  // Pass all players a vector of all enemies.
  spdlog::get(LOGGER)->info("ServerGame::InitializeGame: Setting enemies for each player");
  for (const auto& it : players_) {
    std::vector<Player*> enemies;
    for (const auto& jt : players_)
      if (jt.first != it.first) 
        enemies.push_back(jt.second);
    it.second->set_enemies(enemies);
  }
  
  // Inform players, to start game with initial field included
  CreateAndSendTransferToAllPlayers(0, false);

  // Start ai-threads for all ai-players.
  status_ = SETTING_UP;
  if (mode_ != AI_GAME) {
    for (const auto& it : players_) {
      if (IsAi(it.first)) {
        std::thread ai([this, it]() { Thread_Ai(it.first); });
        ai.detach();
      }
    }
    // Start update-shedule.
    std::thread update([this]() { Thread_RenderField(); });
    update.detach();
  }
  return;
}

std::vector<position_t> ServerGame::SetUpField(RandomGenerator* ran_gen) {
  RandomGenerator* ran_gen_1 = new RandomGenerator(audio_.analysed_data(), &RandomGenerator::ran_boolean_minor_interval);
  RandomGenerator* ran_gen_2 = new RandomGenerator(audio_.analysed_data(), &RandomGenerator::ran_level_peaks);
  // Create field.
  field_ = nullptr;
  spdlog::get(LOGGER)->info("ServerGame::InitializeGame: creating map. field initialized? {}", field_ != nullptr);
  std::vector<position_t> nucleus_positions;
  int denseness = 0;
  while (!field_ && denseness < 4) {
    field_ = new Field(lines_, cols_, ran_gen);
    field_->AddHills(ran_gen_1, ran_gen_2, denseness++);
    field_->BuildGraph();
    nucleus_positions = field_->AddNucleus(max_players_);
    if (nucleus_positions.size() == 0) {
      delete field_;
      field_ = nullptr;
    }
  }
  // Delete random generators user for creating map.
  delete ran_gen_1;
  delete ran_gen_2;
  // Check if map is playable (all nucleus-positions could be found)
  if (!field_) {
    nlohmann::json msg = {{"command", "print_msg"}, {"data", {{"msg", "Game cannot be played with this song, "
      "as map is unplayable. It might work with a higher resolution. (dissonance -r)"}} }};
    SendMessageToAllPlayers(msg);
  }
  spdlog::get(LOGGER)->info("ServerGame::InitializeGame: successfully created map.");
  return nucleus_positions;
}

bool ServerGame::RunAiGame() {
  // Get audio-data for map and all ais..
  auto data_per_beat = audio_.analysed_data().data_per_beat_;
  std::map<std::string, std::list<AudioDataTimePoint>> audio_ais;
  for (const auto& it : players_)
    audio_ais[it.first] = it.second->data_per_beat();

  std::map<std::string, int> time_analysis;

  // Main-loop going through all beats in song once.
  int counter=0;
  while(data_per_beat.size() > 0) {
    // Get next beat.
    auto start_time_stamps = std::chrono::steady_clock::now();
    auto data_at_beat = data_per_beat.front();
    data_per_beat.pop_front();
    time_analysis["TimeStamps"]+=utils::GetElapsedNano(start_time_stamps, std::chrono::steady_clock::now());

    // Main actions
    for (const auto& ai : players_) {
      // IncreaseResources (ai gets double resources)
      ai.second->IncreaseResources(audio_.MoreOffNotes(data_at_beat));
      ai.second->IncreaseResources(audio_.MoreOffNotes(data_at_beat));
      // Get next audio-data and if empty, reset to beginning.
      auto start_time_stamps_ais = std::chrono::steady_clock::now();
      auto data_at_beat_ai = audio_ais[ai.first].front();
      audio_ais[ai.first].pop_front();
      if (audio_ais[ai.first].size() == 0)
        audio_ais[ai.first] = ai.second->data_per_beat();
      time_analysis["TimeStampsAis"]+=utils::GetElapsedNano(start_time_stamps_ais, std::chrono::steady_clock::now());
      // Do action and set last time-point.
      auto start = std::chrono::steady_clock::now();
      ai.second->DoAction(data_at_beat_ai);
      time_analysis["DoAction"]+=utils::GetElapsedNano(start, std::chrono::steady_clock::now());
      ai.second->set_last_time_point(data_at_beat);
    }

    // Movement actions: repeat eight time for every beat.
    for (int i=0; i<8; i++) {
      // Move potentials
      auto start_move = std::chrono::steady_clock::now();
      for (const auto& ai : players_)
        ai.second->MovePotential();
      time_analysis["MovePotential"]+=utils::GetElapsedNano(start_move, std::chrono::steady_clock::now());
      // Check if one player has lost.
      for (const auto& ai : players_) {
        if (ai.second->HasLost()) {
          std::cout << "Time analysis: " << std::endl;
          for (const auto& it : time_analysis) {
            std::cout << it.first << ": " << it.second << std::endl; 
          }
          return true;
        }
      }
      auto start_def_1 = std::chrono::steady_clock::now();
      // Handle def and ipsp-swallow.
      if (counter++%4 == 0) {
        for (const auto& ai : players_) {
          ai.second->HandleDef();
        }
      }
      time_analysis["Def 1"]+=utils::GetElapsedNano(start_def_1, std::chrono::steady_clock::now());
      auto start_def_2 = std::chrono::steady_clock::now();
      for (const auto& ai : players_) {
        for (const auto& it : ai.second->GetIpspAtPosition())
          IpspSwallow(it.first, ai.second, ai.second->enemies());
      }
      time_analysis["Def 2"]+=utils::GetElapsedNano(start_def_2, std::chrono::steady_clock::now());
    }
  }
  std::cout << "Time analysis: " << std::endl;
  for (const auto& it : time_analysis) {
    std::cout << it.first << ": " << it.second/1000000 << std::endl; 
  }
  return false;
}

void ServerGame::Thread_RenderField() {
  spdlog::get(LOGGER)->info("Game::Thread_RenderField: started");
  // Set up audio data and initialize render-frequency (8 times per beat).
  auto audio_start_time = std::chrono::steady_clock::now();
  std::list<AudioDataTimePoint> data_per_beat = audio_.analysed_data().data_per_beat_;
  auto last_update = std::chrono::steady_clock::now();
  auto data_at_beat = data_per_beat.front();
  double render_frequency = 60000.0/(data_at_beat.bpm_*8);

  // Main loop.
  int counter=0;
  while (status_ < CLOSING) {
    if (pause_) continue;
    auto cur_time = std::chrono::steady_clock::now();
    // Analyze audio data.
    std::shared_lock sl(mutex_pause_);
    if (utils::GetElapsed(audio_start_time, cur_time)-time_in_pause_ >= data_at_beat.time_) {
      sl.unlock();
      // Update render-frequency.
      render_frequency = 60000.0/(data_at_beat.bpm_*8);
      data_per_beat.pop_front();
      // All players lost, because time is up:
      if (data_per_beat.size() == 0) {
        status_ = CLOSING;
        // If multi player, inform other player.
        std::map<std::string, nlohmann::json> statistics;
        for (const auto& it : players_) 
          statistics[it.first] = it.second->GetFinalStatistics();
        nlohmann::json resp = {{"command", "game_end"}, {"data", {{"msg", "YOU LOST - times up"},
          {"statistics", statistics}} }};
        SendMessageToAllPlayers(resp);
        break; 
      }
      else 
        data_at_beat = data_per_beat.front();
      // Increase resources for all non-ai players.
      std::unique_lock ul(mutex_players_);
      for (const auto& it : human_players_)
        it.second->IncreaseResources(audio_.MoreOffNotes(data_at_beat));
    }
    // Move potential
    if (utils::GetElapsed(last_update, cur_time) > render_frequency) {
      // Move potentials of all players.
      std::unique_lock ul(mutex_players_);
      for (const auto& it : players_) {
        it.second->MovePotential();
        // Send newly created loophols to player.
        if (!IsAi(it.first)) {
          auto positions = it.second->GetAllPositionsOfNeurons(LOOPHOLE);
          if (positions.size() > 0) {
            std::map<position_t, int> loophols;
            for (const auto& it : positions) 
              loophols[it] = LOOPHOLE;
            nlohmann::json msg = {{"command", "set_units"}, {"data", {{"neurons", loophols}, 
              {"color", it.second->color()}} }};
            ws_server_->SendMessage(it.first, msg);
          }
        }
      }
      // After potentials where move check if a new player has lost and whether a player has scouted new enemy neuerons
      HandlePlayersLost();
      SendScoutedNeurons();
      // Handle actiaved-neurons of all players.
      if (counter++%2 == 0) {
        for (const auto& it : players_)
          it.second->HandleDef();
      }
      // Create player agnostic transfer-data
      CreateAndSendTransferToAllPlayers(1-(static_cast<float>(data_per_beat.size())
            /audio_.analysed_data().data_per_beat_.size()));
      // Refresh page
      last_update = cur_time;
    }
  } 
  std::unique_lock ul(mutex_status_);
  // Clean up
  delete field_;
  for (const auto& it : players_)
    delete players_[it.first];
  status_ = CLOSED;
  spdlog::get(LOGGER)->info("Game::Thread_RenderField: ended");
}

void ServerGame::Thread_Ai(std::string username) {
  spdlog::get(LOGGER)->info("Game::Thread_Ai: started: {}", username);
  auto audio_start_time = std::chrono::steady_clock::now();
  std::list<AudioDataTimePoint> data_per_beat = players_.at(username)->data_per_beat();
  Player* ai = players_.at(username);

  // Handle building neurons and potentials.
  auto data_at_beat = data_per_beat.front();
  while(ai && !ai->HasLost() && status_ < CLOSING) {
    if (pause_) continue;
    auto cur_time = std::chrono::steady_clock::now();
    // Analyze audio data.
    std::shared_lock sl(mutex_pause_);
    if (utils::GetElapsed(audio_start_time, cur_time)-time_in_pause_ >= data_at_beat.time_) {
      spdlog::get(LOGGER)->flush();
      sl.unlock();
      // Do action.
      std::unique_lock ul(mutex_players_);
      ai->DoAction(data_at_beat);
      ai->set_last_time_point(data_at_beat);
      // Increase reasources twice every beat.
      ai->IncreaseResources(audio_.MoreOffNotes(data_at_beat));
      // if (!tutorial_) 
      ai->IncreaseResources(audio_.MoreOffNotes(data_at_beat));
      // Update render-frequency.
      data_per_beat.pop_front();
      // If all beats have been used, restart at beginning.
      if (data_per_beat.size() == 0) {
        spdlog::get(LOGGER)->debug("AI audio-data done. Resetting... {}", players_.at(username)->data_per_beat().size());
        data_per_beat = players_.at(username)->data_per_beat();
        audio_start_time = std::chrono::steady_clock::now();
      }
      else 
        data_at_beat = data_per_beat.front();
    }
  }
  spdlog::get(LOGGER)->info("Game::Thread_Ai: ended");
}

std::map<position_t, std::pair<std::string, int>> ServerGame::GetAndUpdatePotentials() {
  std::map<position_t, std::pair<std::string, int>> potential_per_pos;
  // 1: Swallow epsp potential if on same field as enemies ipsp.
  std::map<position_t, int> positions;
  for (const auto& player : players_) {
    for (const auto& it : player.second->GetIpspAtPosition())
      IpspSwallow(it.first, player.second, player.second->enemies());
  }
  // 2: Create map of potentials in stacked format.
  for (const auto& it : players_) {
    // Add epsp first
    for (const auto& jt : it.second->GetEpspAtPosition()) {
      std::string symbol = utils::CharToString('a', jt.second-1);
      if (symbol > "z") {
        spdlog::get(LOGGER)->error("Symbol to great: {}", symbol);
        symbol = "z";
      }
      // Always add, if field is not jet occupied.
      if (potential_per_pos.count(jt.first) == 0)
        potential_per_pos[jt.first] = {symbol, it.second->color()};
      // Only overwride entry of other player, if this players units are more
      else if (potential_per_pos[jt.first].first < symbol)
        potential_per_pos[jt.first] = {symbol, it.second->color()};
    }
    // Ipsp always dominates epsp
    for (const auto& jt : it.second->GetIpspAtPosition()) {
      std::string symbol = utils::CharToString('1', jt.second-1);
      if (symbol > "9") {
        spdlog::get(LOGGER)->error("Symbol to great: {}", symbol);
        symbol = "9";
      }
      if (potential_per_pos.count(jt.first) == 0)
        potential_per_pos[jt.first] = {symbol, it.second->color()};
      // Always overwride epsps (digits)
      else if (std::isdigit(potential_per_pos[jt.first].first.front()))
        potential_per_pos[jt.first] = {symbol, it.second->color()};
      // Only overwride entry of other player, if this players units are more
      else if (potential_per_pos[jt.first].first < symbol)
        potential_per_pos[jt.first] = {symbol, it.second->color()};
    }
    // MACRO is always shown on top of epsp and ipsp
    for (const auto& jt : it.second->GetMacroAtPosition()) {
      potential_per_pos[jt.first] = {"0", it.second->color()};
    }
  }
  // Build full map:
  return potential_per_pos;
}

void ServerGame::CreateAndSendTransferToAllPlayers(float audio_played, bool update) {
  // Update potential positions.
  auto updated_potentials = GetAndUpdatePotentials();

  // Create player agnostic transfer-data
  std::map<std::string, std::pair<std::string, int>> players_status;
  std::vector<Player*> vec_players;
  std::map<position_t, int> new_dead_neurons;
  for (const auto& it : players_) {
    players_status[it.first] = {it.second->GetNucleusLive(), it.second->color()};
    vec_players.push_back(it.second);
    for (const auto& it : it.second->new_dead_neurons())
      new_dead_neurons[it.first]= it.second;
  }
  Transfer transfer;
  transfer.set_players(players_status);
  transfer.set_new_dead_neurons(new_dead_neurons);
  transfer.set_audio_played(audio_played);
  
  // Set data for game update (only potentials)
  if (update)
    transfer.set_potentials(updated_potentials);
  // Set data for inital setup (full field and all graph-positions)
  else {
    transfer.set_field(field_->Export(vec_players));
    transfer.set_graph_positions(field_->GraphPositions());
  }

  // Add player-specific transfer-data (resources/ technologies) and send data to player.
  nlohmann::json resp = {{"command", (update) ? "update_game" : "init_game"}, {"data", nlohmann::json()}};
  for (const auto& it : human_players_) {
    transfer.set_resources(it.second->t_resources());
    transfer.set_technologies(it.second->t_technologies());
    transfer.set_build_options(it.second->GetBuildingOptions());
    transfer.set_synapse_options(it.second->GetSynapseOptions());
    if (!update)
      transfer.set_macro(it.second->macro());
    resp["data"] = transfer.json();
    ws_server_->SendMessage(it.first, resp);
  }
  // Send data to all observers.
  resp["data"] = transfer.json();
  for (const auto& it : observers_)
    ws_server_->SendMessage(it, resp);
  // Send all new neurons to obersers.
  SendNeuronsToObservers();
}

void ServerGame::HandlePlayersLost() {
  if (status_ == CLOSING)
    return;
  std::map<std::string, nlohmann::json> statistics;
  for (const auto& it : players_) 
    statistics[it.first] = it.second->GetFinalStatistics();
  // Check if new players have lost.
  for (const auto& it : players_) {
    if (it.second->HasLost() && dead_players_.count(it.first) == 0) {
      dead_players_.insert(it.first);
      // Send message if not AI.
      if (!IsAi(it.first) && ws_server_) {
        nlohmann::json resp = {{"command", "game_end"}, {"data", {{"msg", "YOU LOST"}, {"statistics", statistics}} }};
        ws_server_->SendMessage(it.first, resp);
      }
    }
  }
  // If all but one players have one:
  if (dead_players_.size() == players_.size()-1) {
    nlohmann::json resp = {{"command", "game_end"}, {"data", {{"msg", ""}, {"statistics", statistics}} }};
    for (const auto& it : players_) {
      if (dead_players_.count(it.first) == 0) {
        resp["data"]["msg"] = it.first + " WON";
        // If not AI, send message.
        if (!IsAi(it.first) && ws_server_)
          ws_server_->SendMessage(it.first, resp);
      }
    }
    // Also inform all obersers.
    for (const auto& it : observers_) 
      ws_server_->SendMessage(it, resp);
    // Finally end game.
    std::unique_lock ul(mutex_status_);
    status_ = CLOSING;
  }
}

void ServerGame::SendScoutedNeurons() {
  for (const auto& it : human_players_) {
    for (const auto& potential : it.second->GetPotentialPositions()) {
      for (const auto& enemy : it.second->enemies()) {
        for (const auto& nucleus : enemy->GetAllPositionsOfNeurons(NUCLEUS)) {
          if (utils::Dist(potential, nucleus) < enemy->cur_range()) {
            nlohmann::json resp = {{"command", "set_units"}, {"data", {{"neurons", 
              enemy->GetAllNeuronsInRange(nucleus)}, {"color", enemy->color()}} }};
            ws_server_->SendMessage(it.first, resp);
          }
        }
      }
    }
  }
}

void ServerGame::SendNeuronsToObservers() {
  if (observers_.size() == 0)
    return;
  // Iterate through (playing) players and send all new neurons to obersers.
  for (const auto& it : players_) {
    for (const auto& enemy : it.second->enemies()) {
      nlohmann::json resp = {{"command", "set_units"}, {"data", {{"neurons", 
        enemy->new_neurons()}, {"color", enemy->color()}} }}; // TODO (fux): only works for 1 player (new-neurons cleared)
      for (const auto& it : observers_)
        ws_server_->SendMessage(it, resp);
    }
  }
}

void ServerGame::SendMessageToAllPlayers(nlohmann::json msg, std::string ignore_username) {
  spdlog::get(LOGGER)->debug("ServerGame::SendMessageToAllPlayers: num human players: {}", human_players_.size());
  for (const auto& it : human_players_) {
    if (ignore_username == "" || it.first != ignore_username)
      ws_server_->SendMessage(it.first, msg);
  }
}

void ServerGame::IpspSwallow(position_t ipsp_pos, Player* player, std::vector<Player*> enemies) {
  std::string ipsp_id = player->GetPotentialIdIfPotential(ipsp_pos, IPSP);
  for (const auto& enemy : enemies) {
    std::string id = enemy->GetPotentialIdIfPotential(ipsp_pos);
    if (id.find("epsp") != std::string::npos || id.find("macro_1") != std::string::npos) {
      player->NeutralizePotential(ipsp_id, -1); // increase potential by one
      if (enemy->NeutralizePotential(id, 1)) // decrease potential by one
        player->statistics().AddEpspSwallowed();
    }
  }
}

std::string ServerGame::GetMissingResourceStr(Costs missing_costs) {
  std::string res = "";
  if (missing_costs.size() == 0)
    return res;
  for (const auto& it : missing_costs)
    res += "Missing " + std::to_string(it.second) + " " + resources_name_mapping.at(it.first) + "! ";
  return res;
}
