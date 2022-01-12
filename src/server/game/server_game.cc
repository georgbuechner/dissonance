#include "server/game/server_game.h"
#include "nlohmann/json_fwd.hpp"
#include "server/game/player/player.h"
#include "share/constants/codes.h"
#include "share/defines.h"
#include "share/objects/dtos.h"
#include "share/objects/transfer.h"
#include "share/objects/units.h"
#include "server/websocket/websocket_server.h"
#include "share/tools/eventmanager.h"
#include "share/tools/utils/utils.h"

#include "spdlog/spdlog.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <mutex>
#include <string>
#include <thread>
#include <unistd.h>

ServerGame::ServerGame(int lines, int cols, int mode, int num_players, std::string base_path, WebsocketServer* srv) 
    : num_players_(num_players), audio_(base_path), ws_server_(srv), status_(WAITING), mode_(mode), lines_(lines), cols_(cols) {
  // Initialize eventmanager.
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
  eventmanager_.AddHandler("set_ipsp_target", &ServerGame::m_SetIpspTarget);
  eventmanager_.AddHandler("set_epsp_target", &ServerGame::m_SetEpspTarget);
}

int ServerGame::status() {
  return status_;
}

int ServerGame::mode() {
  return mode_;
}

void ServerGame::set_status(int status) {
  std::unique_lock ul(mutex_status_);
  status_ = status;
}

void ServerGame::AddPlayer(std::string username, int lines, int cols) {
  std::unique_lock ul(mutex_players_);
  spdlog::get(LOGGER)->info("ServerGame::AddPlayer: {}", username);
  // Check is free slots in lobby.
  if (players_.size() < num_players_) {
    spdlog::get(LOGGER)->debug("ServerGame::AddPlayer: adding user.");
    players_[username] = nullptr;
    // Adjust field size and width
    lines_ = (lines < lines_) ? lines : lines_;
    cols_ = (cols < cols_) ? cols : cols_;
  }
  // Only start game if status is still waiting, to avoid starting game twice.
  if (players_.size() >= num_players_ && status_ == WAITING_FOR_PLAYERS) {
    spdlog::get(LOGGER)->debug("ServerGame::AddPlayer: starting game.");
    ul.unlock();
    StartGame();
  }
}

nlohmann::json ServerGame::HandleInput(std::string command, nlohmann::json msg) {
  if (eventmanager_.handlers().count(command))
    (this->*eventmanager_.handlers().at(command))(msg);
  else 
    msg = nlohmann::json();
  spdlog::get(LOGGER)->debug("ClientGame::HandleAction: response {}", msg.dump());
  return msg;
}

// command methods

void ServerGame::m_AddIron(nlohmann::json& msg) {
  std::string username = msg["username"];
  Player* player = (players_.count(username) > 0) ? players_.at(username) : NULL;
  int resource = msg["data"]["resource"];
  if (player && player->DistributeIron(resource)) {
    msg = {{"command", "set_msg"}, {"data", {{"msg", "Distribute iron: done!"}} }};
    spdlog::get(LOGGER)->debug("ServerGame::m_AddIron: checking whether to send player new resource-neuron: {} cur: {}", 
        resource, player->resources().at(resource).distributed_iron());
    // If resource is newly created, send client resource-neuron as new unit.
    if (player->resources().at(resource).distributed_iron() == 2) {
      spdlog::get(LOGGER)->debug("ServerGame::m_AddIron: sending player new resource-neuron");
      position_t pos = player->resources().at(resource).pos();
      spdlog::get(LOGGER)->debug("ServerGame::m_AddIron: 1");
      nlohmann::json req = {{"command", "set_unit"}, {"data", {{"unit", RESOURCENEURON}, {"pos", pos}, 
          {"color", COLOR_RESOURCES}} }};
      spdlog::get(LOGGER)->debug("ServerGame::m_AddIron: 2");
      ws_server_->SendMessage(username, req.dump());
      spdlog::get(LOGGER)->debug("ServerGame::m_AddIron: 3");
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
    // If resource is newly created, send client resource-neuron as new unit.
    if (player->resources().at(resource).bound() == 1) {
      spdlog::get(LOGGER)->debug("ServerGame::m_AddIron: sending player removed resource-neuron");
      position_t pos = player->resources().at(resource).pos();
      nlohmann::json req = {{"command", "set_unit"}, {"data", {{"unit", RESOURCENEURON}, {"pos", pos}, 
          {"color", COLOR_DEFAULT}} }};
      ws_server_->SendMessage(msg["username"], req.dump());
    }
  }
  else 
    msg = {{"command", "set_msg"}, {"data", {{"msg", "Remove iron: not enough iron!"}} }};
}

void ServerGame::m_AddTechnology(nlohmann::json& msg) {
  Player* player = (players_.count(msg["username"]) > 0) ? players_.at(msg["username"]) : NULL;
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
  nlohmann::json resp = {{"command", "game_end"}, {"data", {{"msg", "YOU WON - opponent resigned"}} }};
  SendMessageToAllPlayers(resp.dump(), msg["username"]);
  msg = nlohmann::json();
}

void ServerGame::m_CheckBuildNeuron(nlohmann::json& msg) {
  int unit = msg["data"]["unit"];
  Player* player = (players_.count(msg["username"]) > 0) ? players_.at(msg["username"]) : NULL;
  if (player) {
    std::string missing = GetMissingResourceStr(player->GetMissingResources(unit));
    std::vector<position_t> positions = player->GetAllPositionsOfNeurons(NUCLEUS);
    // Can be build (enough resources) and start-position for selecting pos is known (only one nucleus)
    if (missing == "" && positions.size() == 1)
      msg = {{"command", "build_neuron"}, {"data", {{"unit", unit}, {"start_pos", positions.front()}, 
        {"range", player->cur_range()}} }}; 
    // Can be build (enough resources) and start-position for selecting pos is unknown (multiple nucleus)
    else if (missing == "") 
      msg = {{"command", "build_neuron"}, {"data", {{"unit", unit}, {"positions", positions}, 
        {"range", player->cur_range()}} }}; 
    else 
      msg = {{"command", "set_msg"}, {"data", {{"msg", "Not enough resource! missing: " + missing}}}};
  }
}

void ServerGame::m_CheckBuildPotential(nlohmann::json& msg) {
  int unit = msg["data"]["unit"];
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
      BuildPotentials(unit, pos, msg["data"]["num"], msg["username"], player);
      msg = nlohmann::json();
    }
    // More than one synapse and position not given => tell user to select position.
    else  
      msg = {{"command", "build_potential"}, {"data", {{"unit", unit}, {"positions", 
        player->GetAllPositionsOfNeurons(SYNAPSE)}, {"num", msg["data"]["num"]}} }}; 
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
}

void ServerGame::m_GetPositions(nlohmann::json& msg) {
  Player* player = (players_.count(msg["username"]) > 0) ? players_.at(msg["username"]) : NULL;
  spdlog::get(LOGGER)->debug("ServerGame::m_GetPositions: dezerialising dto.");
  GetPosition req = GetPosition(msg);
  spdlog::get(LOGGER)->debug("ServerGame::m_GetPositions: dezerialising dto done.");
  if (player) {
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
    msg = {{"command", msg["data"]["return_cmd"]}, {"data", {{"positions", all_positions}} }};
  }
}

void ServerGame::m_ToggleSwarmAttack(nlohmann::json& msg) {
  Player* player = (players_.count(msg["username"]) > 0) ? players_.at(msg["username"]) : NULL;
  if (player) {
    std::string on_off = (player->SwitchSwarmAttack(utils::PositionFromVector(msg["data"]["pos"]))) ? "on" : "off";
    msg = {{"command", "set_msg"}, {"data", {{"msg", "Toggle swarm-attack successfull. Swarm attack " + on_off}} }};
  }
}

void ServerGame::m_SetWayPoint(nlohmann::json& msg) {
  Player* player = (players_.count(msg["username"]) > 0) ? players_.at(msg["username"]) : NULL;
  if (player) {
    int num = msg["data"]["num"];
    int tech = player->technologies().at(WAY).first;
    position_t synapse_pos = utils::PositionFromVector(msg["data"]["synapse_pos"]);
    std::string x_of = std::to_string(num) + "/" + std::to_string(tech);
    if (num == 1) 
      player->ResetWayForSynapse(synapse_pos, msg["data"]["pos"]);
    else 
      player->AddWayPosForSynapse(synapse_pos, msg["data"]["pos"]);
    msg = {{"command", "set_wps"}, {"data", {{"msg", "New way-point added: " + x_of}, {"synapse_pos", synapse_pos}} }};
    msg["data"]["num"] = (num < tech) ? num+1 : -1;  // indicate setting next way-point or that last way-point was set.
  }
}

void ServerGame::m_SetIpspTarget(nlohmann::json& msg) {
  Player* player = (players_.count(msg["username"]) > 0) ? players_.at(msg["username"]) : NULL;
  if (player) {
    player->ChangeIpspTargetForSynapse(msg["data"]["synapse_pos"], msg["data"]["pos"]);
    msg = {{"command", "set_msg"}, {"data", {{"msg", "Ipsp target for this synapse set"}} }};
  }
}

void ServerGame::m_SetEpspTarget(nlohmann::json& msg) {
  Player* player = (players_.count(msg["username"]) > 0) ? players_.at(msg["username"]) : NULL;
  if (player) {
    player->ChangeEpspTargetForSynapse(msg["data"]["synapse_pos"], msg["data"]["pos"]);
    msg = {{"command", "set_msg"}, {"data", {{"msg", "Epsp target for this synapse set"}} }};
  }
}


void ServerGame::BuildPotentials(int unit, position_t pos, int num_potenials_to_build, 
    std::string username, Player* player) {
  bool success = false;
  for (int i=0; i < num_potenials_to_build; i++) {
    success = player->AddPotential(pos, unit);
    if (!success)
      break;
    // Wait a bit.
    utils::WaitABit(110);
  }
  nlohmann::json msg = {{"command", "set_msg"}, {"data", {{"msg", "Success!"}} }};
  if (!success)
    msg["data"]["msg"] = "Failed!";
  ws_server_->SendMessage(username, msg.dump());
}

void ServerGame::m_InitializeGame(nlohmann::json& msg) {
  std::string username = msg["username"];
  spdlog::get(LOGGER)->info("ServerGame::InitializeGame: initializing with user: {}", username);
  spdlog::get(LOGGER)->flush();
  nlohmann::json data = msg["data"];

  std::string source_path = data["source_path"];
  spdlog::get(LOGGER)->info("ServerGame::InitializeGame: Selected path: {}", source_path);
  audio_.set_source_path(source_path);
  audio_.Analyze();

  // Add host to players.
  players_[username] = nullptr;

  // If SINGLE_PLAYER, add AI to players.
  if (mode_ == SINGLE_PLAYER) {
    players_["AI"] = nullptr;
    StartGame();
  }
  // Otherwise send info "waiting for players" to host.
  else {
    status_ = WAITING_FOR_PLAYERS;
    msg["command"] = "print_msg";
    msg["data"] = {{"msg", "Wainting for players..."}};
  }
}

void ServerGame::StartGame() {
  // Build field.
  RandomGenerator* ran_gen = new RandomGenerator(audio_.analysed_data(), &RandomGenerator::ran_note);
  RandomGenerator* map_1 = new RandomGenerator(audio_.analysed_data(), &RandomGenerator::ran_boolean_minor_interval);
  RandomGenerator* map_2 = new RandomGenerator(audio_.analysed_data(), &RandomGenerator::ran_level_peaks);
  auto intervals_ = audio_.analysed_data().intervals_;
  int denceness = 0;
  std::vector<position_t> nucleus_positions;
  spdlog::get(LOGGER)->info("ServerGame::InitializeGame: creating map {} ", denceness);
  while (nucleus_positions.size() == 0 && denceness < 5) {
    spdlog::get(LOGGER)->info("ServerGame::InitializeGame: creating map try: {}", denceness);
    spdlog::get(LOGGER)->debug("ServerGame::InitializeGame: creating hills}");
    // Create field with hills
    field_ = new Field(lines_, cols_, ran_gen);
    field_->AddHills(map_1, map_2, denceness++);
    // Create a nucleus for each player.
    for (unsigned int i=0; i<num_players_; i++) {
      int section = (intervals_[i].darkness_ + intervals_[i].notes_out_key_) % 8 + 1;
      nucleus_positions.push_back(field_->AddNucleus(section));
    }
    spdlog::get(LOGGER)->debug("ServerGame::InitializeGame: builing graph...");
    // Build graph.
    try {
      field_->BuildGraph(nucleus_positions);
    } catch (std::logic_error& e) {
      spdlog::get(LOGGER)->warn("Game::play: graph could not be build: {}", e.what());
      field_ = nullptr;
      nucleus_positions.clear();
      continue;
    }
  }
  if (!field_) {
    nlohmann::json msg = {{"command", "print_msg"}, {"data", {{"msg", 
      "Game cannot be played with this song, as map is unplayable. "
      "It might work with a higher resolution. (dissonance -r)"}} }};
    SendMessageToAllPlayers(msg.dump());
    return;
  }
  spdlog::get(LOGGER)->info("ServerGame::InitializeGame: successfully created map.");

  // Setup players.
  spdlog::get(LOGGER)->info("ServerGame::InitializeGame: Creating {} players", nucleus_positions.size());
  unsigned int counter = 0;
  for (const auto& it : players_) {
    int color = (counter % 4) + 10;
    if (it.first == "AI") {
      players_[it.first] = new AudioKi(nucleus_positions[counter], field_, &audio_, ran_gen);
      // Setup audio-ki
      spdlog::get(LOGGER)->info("ServerGame::InitializeGame: SINGLE_PLAYER: settup AI tactics");
      players_.at("AI")->SetUpTactics(true); 
      players_.at("AI")->DistributeIron(Resources::OXYGEN);
      players_.at("AI")->DistributeIron(Resources::OXYGEN);
      players_.at("AI")->HandleIron(audio_.analysed_data().data_per_beat_.front());
    }
    else {
      // If single-player, always use standard-player color.
      if (mode_ == SINGLE_PLAYER)
        color = COLOR_PLAYER;
      players_[it.first] = new Player(nucleus_positions[counter], field_, ran_gen, color);
    }
    counter++;
  }
  // Pass all players a vector of all enemies.
  spdlog::get(LOGGER)->info("ServerGame::InitializeGame: Setting enemies for each player");
  for (const auto& it : players_) {
    std::vector<Player*> enemies;
    for (const auto& jt : players_) {
      if (jt.first != it.first) enemies.push_back(jt.second);
    }
    it.second->set_enemies(enemies);
  }

  // Start two main threads.
  status_ = SETTING_UP;
  // If single-player, start ai.
  if (mode_ == SINGLE_PLAYER) {
    std::thread ai([this]() { Thread_Ai(); });
    ai.detach();
  }
  // Start update-shedule.
  std::thread update([this]() { Thread_RenderField(); });
  update.detach();
  // Inform players, to start game with initial field included
  CreateAndSendTransferToAllPlayers(0, false);
  return ;
}

void ServerGame::Thread_RenderField() {
  spdlog::get(LOGGER)->info("Game::Thread_RenderField: started");
  auto audio_start_time = std::chrono::steady_clock::now();
  auto analysed_data = audio_.analysed_data();
  std::list<AudioDataTimePoint> data_per_beat = analysed_data.data_per_beat_;
  size_t total_audio_length = data_per_beat.size();

  auto last_update = std::chrono::steady_clock::now();
  auto last_resource_player_one = std::chrono::steady_clock::now();
  auto last_resource_player_two = std::chrono::steady_clock::now();

  double ki_resource_update_frequency = data_per_beat.front().bpm_;
  double player_resource_update_freqeuncy = data_per_beat.front().bpm_;
  double render_frequency = 40;

  bool off_notes = false;
  spdlog::get(LOGGER)->debug("Game::Thread_RenderField: setup finished, starting main-loop");
 
  while (status_ < CLOSING) {
    auto cur_time = std::chrono::steady_clock::now();

    // Analyze audio data.
    auto elapsed = utils::GetElapsed(audio_start_time, cur_time);
    auto data_at_beat = data_per_beat.front();
    if (elapsed >= data_at_beat.time_) {
      spdlog::get(LOGGER)->debug("Game::RenderField: next data_at_beat");

      render_frequency = 60000.0/(data_at_beat.bpm_*16);
      ki_resource_update_frequency = (60000.0/data_at_beat.bpm_); //*(data_at_beat.level_/50.0);
      player_resource_update_freqeuncy = 60000.0/(static_cast<double>(data_at_beat.bpm_)/2);
    
      off_notes = audio_.MoreOffNotes(data_at_beat);
      data_per_beat.pop_front();
    }

    // All players lost, because time is up:
    if (data_per_beat.size() == 0) {
      nlohmann::json resp = {{"command", "game_end"}, {"data", {{"msg", "YOU LOST - times up"}} }};
      SendMessageToAllPlayers(resp.dump());
      break;
    }

    // Send message to all players, which have lost (don't worry about player no
    // longer connected at this point, handler by webserver). And count all
    // players, which have lost and store the last player which has not lost.
    unsigned int num_players_lost = 0;
    std::string player_not_lost = "";
    for (const auto& it : players_) {
      if (it.second->HasLost()) {
        num_players_lost++;
        if (it.first != "AI") {
          nlohmann::json resp = {{"command", "game_end"}, {"data", {{"msg", "YOU LOST"}} }};
          ws_server_->SendMessage(it.first, resp.dump());
        }
      }
      else 
        player_not_lost = it.first;
    }

    // If number of player which have lost is one less than total of number of
    // players, then one player has wone.
    if (num_players_lost == players_.size() -1) {
      nlohmann::json resp = {{"command", "game_end"}, {"data", {{"msg", "YOU WON"}} }};
      ws_server_->SendMessage(player_not_lost, resp.dump());
      {
        std::unique_lock ul(mutex_status_);
        status_ = CLOSING;
      }
      break;
    }

    // Increase resources for all human players at the same time.
    if (utils::GetElapsed(last_resource_player_one, cur_time) > player_resource_update_freqeuncy) {
      for (const auto& it : players_) {
        if (it.first != "AI") {
          it.second->IncreaseResources(off_notes);
          last_resource_player_one = cur_time;
        }
      }
    }
    // Increase resources of AI (only but then always existing in SINGLE_PLAYER mode)
    if (utils::GetElapsed(last_resource_player_two, cur_time) > ki_resource_update_frequency 
        && mode_ == SINGLE_PLAYER) {
      players_.at("AI")->IncreaseResources(off_notes);
      last_resource_player_two = cur_time;
    }

    // Move potential
    if (utils::GetElapsed(last_update, cur_time) > render_frequency) {
      // Move player soldiers and check if enemy den's lp is down to 0.
      for (const auto& it : players_)
        it.second->MovePotential();
      // Remove enemy soldiers in renage of defence towers.
      for (const auto& it : players_)
        it.second->HandleDef();

      // Check if neuron has reached view of enemy:
      for (const auto& it : players_) {
        for (const auto& potential : it.second->GetPotentialPositions()) {
          for (const auto& enemy : it.second->enemies()) {
            for (const auto& nucleus : enemy->GetAllPositionsOfNeurons(NUCLEUS)) {
              if (utils::Dist(potential, nucleus) < enemy->cur_range()) {
                nlohmann::json resp = {{"command", "set_units"}, {"data", {{"neurons", 
                  enemy->GetAllNeuronsInRange(nucleus)}, {"color", enemy->color()}} }};
                ws_server_->SendMessage(it.first, resp.dump());
              }
            }
          }
        }
      }

      // Create player agnostic transfer-data
      CreateAndSendTransferToAllPlayers(1-(static_cast<float>(data_per_beat.size())/total_audio_length));

      // Refresh page
      last_update = cur_time;
    }
  } 
  sleep(1);
  std::unique_lock ul(mutex_status_);
  status_ = CLOSED;
  spdlog::get(LOGGER)->info("Game::Thread_RenderField: ended");
}

void ServerGame::Thread_Ai() {
  spdlog::get(LOGGER)->info("Game::Thread_Ai: started");
  auto audio_start_time = std::chrono::steady_clock::now();
  auto analysed_data = audio_.analysed_data();
  std::list<AudioDataTimePoint> data_per_beat = analysed_data.data_per_beat_;

  // Handle building neurons and potentials.
  while(status_ < CLOSING) {
    // Analyze audio data.
    auto elapsed = utils::GetElapsed(audio_start_time, std::chrono::steady_clock::now());
    if (data_per_beat.size() == 0)
      continue;
    auto data_at_beat = data_per_beat.front();
    if (elapsed >= data_at_beat.time_) {
      players_.at("AI")->DoAction(data_at_beat);
      players_.at("AI")->set_last_time_point(data_at_beat);
      data_per_beat.pop_front();
    }
  }
  spdlog::get(LOGGER)->info("Game::Thread_Ai: ended");
}

std::map<position_t, std::pair<std::string, int>> ServerGame::GetAndUpdatePotentials() {
  std::map<position_t, std::pair<std::string, int>> potential_per_pos;

  // 1: Swallow epsp potential if on same field as enemies ipsp.
  std::map<position_t, int> positions;
  for (const auto& player : players_) {
    positions = player.second->GetIpspAtPosition();
    for (const auto& it : positions)
      IpspSwallow(it.first, player.second, player.second->enemies());
  }

  // 2: Create map of potentials in stacked format.
  for (const auto& it : players_) {
    // Add epsp first
    for (const auto& jt : it.second->GetEpspAtPosition()) {
      std::string symbol = utils::CharToString('a', jt.second-1);
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
      if (potential_per_pos.count(jt.first) == 0)
        potential_per_pos[jt.first] = {symbol, it.second->color()};
      // Always overwride epsps (digits)
      else if (std::isdigit(potential_per_pos[jt.first].first.front()))
        potential_per_pos[jt.first] = {symbol, it.second->color()};
      // Only overwride entry of other player, if this players units are more
      else if (potential_per_pos[jt.first].first < symbol)
        potential_per_pos[jt.first] = {symbol, it.second->color()};
    }
  }

  // Build full map:
  return potential_per_pos;
}

void ServerGame::CreateAndSendTransferToAllPlayers(float audio_played, bool update) {
  spdlog::get(LOGGER)->debug("ServerGame::CreateAndSendTransferToAllPlaters: {}, {}", audio_played, update);
  // Create player agnostic transfer-data
  Transfer transfer;
  std::map<std::string, std::string> players_status;
  std::vector<Player*> vec_players;
  std::map<position_t, int> new_dead_neurons;
  for (const auto& it : players_) {
    players_status[it.first] = it.second->GetNucleusLive();
    vec_players.push_back(it.second);
    for (const auto& it : it.second->new_dead_neurons())
      new_dead_neurons[it.first]= it.second;
  }
  transfer.set_players(players_status);
  if (update)
    transfer.set_potentials(GetAndUpdatePotentials());
  else 
    transfer.set_field(field_->Export(vec_players));
  transfer.set_audio_played(audio_played);

  std::string command = (update) ? "update_game" : "init_game";
  nlohmann::json resp = {{"command", command}, {"data", nlohmann::json()}};

  // Add player-specific transfer-data (resources/ technologies) and send data to player.
  for (const auto& it : players_) {
    if (it.first != "AI") {
      transfer.set_resources(it.second->t_resources());
      transfer.set_technologies(it.second->t_technologies());
      transfer.set_new_dead_neurons(new_dead_neurons);
      resp["data"] = transfer.json();
      ws_server_->SendMessage(it.first, resp.dump());
    }
  }
}

void ServerGame::SendMessageToAllPlayers(std::string msg, std::string ignore_username) {
  for (const auto& it : players_) {
    if (it.first != "AI" && (ignore_username == "" || it.first != ignore_username))
      ws_server_->SendMessage(it.first, msg);
  }
}

void ServerGame::IpspSwallow(position_t ipsp_pos, Player* player, std::vector<Player*> enemies) {
  std::string ipsp_id = player->GetPotentialIdIfPotential(ipsp_pos, IPSP);
  for (const auto& enemy : enemies) {
    std::string id = enemy->GetPotentialIdIfPotential(ipsp_pos);
    if (id.find("epsp") != std::string::npos) {
      player->NeutralizePotential(ipsp_id, -1); // increase potential by one
      enemy->NeutralizePotential(id, 1); // decrease potential by one
      spdlog::get(LOGGER)->info("IPSP at {} swallowed esps", utils::PositionToString(ipsp_pos));
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
