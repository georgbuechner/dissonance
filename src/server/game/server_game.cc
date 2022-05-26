#include "server/game/server_game.h"
#include "server/game/player/audio_ki.h"
#include "server/game/player/player.h"
#include "share/audio/audio.h"
#include "share/constants/codes.h"
#include "share/defines.h"
#include "share/objects/units.h"
#include "server/websocket/websocket_server.h"
#include "share/shemes/commands.h"
#include "share/shemes/data.h"
#include "share/tools/eventmanager.h"
#include "share/tools/random/random.h"
#include "share/tools/utils/utils.h"

#include "spdlog/spdlog.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <memory>
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
  eventmanager_.AddHandler("send_audio", &ServerGame::m_SendSong);
  eventmanager_.AddHandler("send_audio_data", &ServerGame::m_AddAudioPart); 
  eventmanager_.AddHandler("initialize_game", &ServerGame::m_InitializeGame);
  eventmanager_.AddHandler("add_iron", &ServerGame::m_AddIron);
  eventmanager_.AddHandler("remove_iron", &ServerGame::m_RemoveIron);
  eventmanager_.AddHandler("add_technology", &ServerGame::m_AddTechnology);
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
    it.second->statistics()->print();
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
    std::shared_ptr<Data> data = std::make_shared<CheckSendAudio>(host_ + "/" + audio_file_name_);
    ws_server_->SendMessage(username, "audio_exists", data);
  }
}

void ServerGame::m_AddAudioPart(std::shared_ptr<Data> data) {
  // Set song name if not already set.
  if (audio_file_name_ == "")
    audio_file_name_ = data->songname();
  std::string path = base_path_ + "data/user-files/" + data->username();
  // Create directory for this user.
  if (!std::filesystem::exists(path))
    std::filesystem::create_directory(path);
  path += "/"+data->songname();
  audio_data_+=data->content();
  if (data->part() == data->parts()) {
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

void ServerGame::HandleInput(std::string command, std::shared_ptr<Data> data) {
  // Check if command is known.
  if (eventmanager_.handlers().count(command)) {
    // Lock mutex! And do player action.
    std::unique_lock ul(mutex_players_);
    try {
      (this->*eventmanager_.handlers().at(command))(data);
    }
    catch(std::exception& e) {
      spdlog::get(LOGGER)->error("ServerGame::HandleInput: failed with exception: {}", e.what());
    }
  }
  else {
    spdlog::get(LOGGER)->warn("ServerGame::HandleInput: unknown command!");
  }
}

// command methods

void ServerGame::m_AddIron(std::shared_ptr<Data> data) {
  // Get username player and user.
  Player* player = GetPlayer(data->u());
  std::string msg = "Distribute iron: not enough iron!";
  if (player->DistributeIron(data->resource())) {
    // If resource is newly created, send client resource-neuron as new unit.
    if (player->resources().at(data->resource()).distributed_iron() == 2) {
      ws_server_->SendMessage(data->u(), "set_unit", std::make_shared<FieldPosition>(
            player->resources().at(data->resource()).pos(), RESOURCENEURON, COLOR_RESOURCES,
            data->resource()));
    }
    msg = "Distribute iron: done!";
  }
  ws_server_->SendMessage(data->u(), "set_msg", std::make_shared<Msg>(msg));
}

void ServerGame::m_RemoveIron(std::shared_ptr<Data> data) {
  Player* player = GetPlayer(data->u());
  std::string msg = "Remove iron: not enough iron!";
  if (player->RemoveIron(data->resource())) {
    // If resource is now below produce-minimum, send client resource-neuron as new unit.
    if (player->resources().at(data->resource()).bound() == 1) {
      ws_server_->SendMessage(data->u(), "set_unit", std::make_shared<FieldPosition>(
            player->resources().at(data->resource()).pos(), RESOURCENEURON, COLOR_DEFAULT, 
            data->resource()));
    }
    msg = "Remove iron: done!";
  }
  ws_server_->SendMessage(data->u(), "set_msg", std::make_shared<Msg>(msg));
}

void ServerGame::m_AddTechnology(std::shared_ptr<Data> data) {
  Player* player = GetPlayer(data->u());
  std::string msg = "Add technology: probably not enough resources!";
  // If player exists, add technology and send mathcing response.
  if (player->AddTechnology(data->technology())) {
    msg = "Add technology: done!";
    ws_server_->SendMessage(data->u(), "add_technology", data); // return data (AddTechnology).
  }
  ws_server_->SendMessage(data->u(), "set_msg", std::make_shared<Msg>(msg));
}

void ServerGame::m_CheckBuildNeuron(std::shared_ptr<Data> data) {
  Player* player = GetPlayer(data->u());
  int unit = data->unit();
  std::string missing = GetMissingResourceStr(player->GetMissingResources(unit));
  // Not enough resources: send error (not-enough-resource) message.
  if (missing != "") {
    std::string msg = "Not enough resource! missing: " + missing;
    ws_server_->SendMessage(data->u(), "set_msg", std::make_shared<Msg>(msg));
  }
  else {
    // Always set range
    data->set_range(player->cur_range()); 
    // Get positions
    std::vector<position_t> positions = (unit == NUCLEUS) ? field_->GetAllCenterPositionsOfSections() 
      : player->GetAllPositionsOfNeurons(NUCLEUS);
    // If only one position, set position
    if (positions.size() == 1)
      data->set_start_pos(positions.front());
    // Otherwise send all positions
    else 
      data->set_positions(positions);
    ws_server_->SendMessage(data->u(), "build_neuron", data); // send updated data (BuildNeuron).
  }
}

void ServerGame::m_CheckBuildPotential(std::shared_ptr<Data> data) {
  Player* player = GetPlayer(data->u());
  int unit = data->unit();
  int num = data->num();
  auto synapses = player->GetAllPositionsOfNeurons(SYNAPSE);
  std::string missing = GetMissingResourceStr(player->GetMissingResources(unit));
  // Not enough resources: send error (not-enough-resource) message.
  if (missing != "") {
    std::string msg = "Not enough resource! missing: " + missing;
    ws_server_->SendMessage(data->u(), "set_msg", std::make_shared<Msg>(msg));
  }
  // No synapses: send error (no-synapses) message.
  else if (synapses.size() == 0) {
    std::string msg = "No synapse!";
    ws_server_->SendMessage(data->u(), "set_msg", std::make_shared<Msg>(msg));
  }
  // Only one synapse or player specified position => add potential
  else if (synapses.size() == 1 || data->synapse_pos() != DEFAULT_POS) {
    position_t pos = (synapses.size() == 1) ? synapses.front() : data->synapse_pos();
    BuildPotentials(unit, pos, num, data->u(), player);
  }
  // More than one synapse and position not given => tell user to select position.
  else  {
    data->set_positions(player->GetAllPositionsOfNeurons(SYNAPSE));
    ws_server_->SendMessage(data->u(), "build_potential", data); // send updated data (BuildNeuron).
  }
}

void ServerGame::m_BuildNeurons(std::shared_ptr<Data> data) {
  Player* player = GetPlayer(data->u());
  int unit = data->unit();
  position_t pos = data->pos();

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
    ws_server_->SendMessage(data->u(), "set_unit", std::make_shared<FieldPosition>(pos, unit, player->color()));
  }
  else
    ws_server_->SendMessage(data->u(), "set_msg", std::make_shared<Msg>("Failed!"));
}

void ServerGame::m_GetPositions(std::shared_ptr<Data> data) {
  spdlog::get(LOGGER)->debug("ServerGame::m_GetPositions. synapse_pos: {}", 
      utils::PositionToString(data->synapse_pos()));
  Player* player = GetPlayer(data->u());
  auto return_data = data->data();
  std::vector<std::vector<position_t>> all_positions;
  for (const auto& it : data->position_requests()) {
    std::vector<position_t> positions;
    // player-units
    if (it.first == Positions::PLAYER)
      return_data->set_player_units(player->GetAllPositionsOfNeurons(it.second._unit));
    // enemy-units
    else if (it.first == Positions::ENEMY) {
      for (const auto& enemy : player->enemies()) 
        for (const auto& it : enemy->GetAllPositionsOfNeurons(it.second._unit))
          positions.push_back(it);
      return_data->set_enemy_units(positions);
    }
    // center-positions
    else if (it.first == Positions::CENTER)
      return_data->set_centered_positions(field_->GetAllCenterPositionsOfSections());
    // ipsp-/ epsp-targets
    else if (it.first == Positions::TARGETS) {
      position_t ipsp_target_pos = player->GetSynapesTarget(it.second._pos, it.second._unit);
      if (ipsp_target_pos.first != -1)
        positions.push_back(ipsp_target_pos);
      return_data->set_target_positions(positions);
    }
    else if (it.first == Positions::CURRENT_WAY) {
      // Add all positions of way to ipsp-target
      for (const auto& way_pos : field_->GetWay(it.second._pos, player->GetSynapesWayPoints(it.second._pos, IPSP)))
        positions.push_back(way_pos);
      // Add all positions of way to epsp-target
      for (const auto& way_pos : field_->GetWay(it.second._pos, player->GetSynapesWayPoints(it.second._pos, EPSP)))
        positions.push_back(way_pos);
      return_data->set_current_way(positions);
    }
    else if (it.first == Positions::CURRENT_WAY_POINTS) {
      return_data->set_current_waypoints(player->GetSynapesWayPoints(it.second._pos));
    }
    all_positions.push_back(positions);
  }
  ws_server_->SendMessage(data->u(), data->return_cmd(), return_data); // send updated data (return_data_
}

void ServerGame::m_ToggleSwarmAttack(std::shared_ptr<Data> data) {
  Player* player = GetPlayer(data->u());
  // Toggle swarm attack and get wether now active/ inactive.
  std::string on_off = (player->SwitchSwarmAttack(data->pos())) ? "on" : "off";
  std::string msg = "Toggle swarm-attack successfull. Swarm attack " + on_off;
  ws_server_->SendMessage(data->u(), "set_msg", std::make_shared<Msg>(msg));
}

void ServerGame::m_SetWayPoint(std::shared_ptr<Data> data) {
  spdlog::get(LOGGER)->debug("ServerGame::m_SetWayPoint. num={}", data->num());
  Player* player = GetPlayer(data->u());
  int tech = player->technologies().at(WAY).first; // number of availible way points to add.
  // If first way-point, reset waypoints with new way-point.
  if (data->num() == 1) 
    player->ResetWayForSynapse(data->synapse_pos(), data->way_point());
  // Otherwise add waypoint.
  else 
    player->AddWayPosForSynapse(data->synapse_pos(), data->way_point());
  // Create new SetWayPoints-Object with same synapse-pos, but updated number and message.
  auto new_data = std::make_shared<SetWayPoints>(data->synapse_pos());
  new_data->set_msg("New way-point added: " + std::to_string(data->num()) + "/" + std::to_string(tech));
  new_data->set_num((data->num() < tech) ? data->num()+1 : 0xFFF);
  spdlog::get(LOGGER)->debug("ServerGame::m_SetWayPoint. Returning with num={}", new_data->num());
  ws_server_->SendMessage(data->u(), "set_wps", new_data);
}

void ServerGame::m_SetTarget(std::shared_ptr<Data> data) {
  Player* player = GetPlayer(data->u());
  // Change ipsp target and response with message
  int unit = data->unit();
  if (unit == IPSP)
    player->ChangeIpspTargetForSynapse(data->synapse_pos(), data->target());
  else if (unit == EPSP)
    player->ChangeEpspTargetForSynapse(data->synapse_pos(), data->target());
  else if (unit == MACRO)
    player->ChangeMacroTargetForSynapse(data->synapse_pos(), data->target());
  ws_server_->SendMessage(data->u(), "set_msg", std::make_shared<Msg>("Target for this synapse set"));
}

void ServerGame::m_SetPauseOn(std::shared_ptr<Data>) {
  spdlog::get(LOGGER)->info("Set pause on");
  pause_ = true; 
  pause_start_ = std::chrono::steady_clock::now();
}

void ServerGame::m_SetPauseOff(std::shared_ptr<Data>) {
  pause_ = false;
  std::unique_lock ul(mutex_pause_);
  time_in_pause_ += utils::GetElapsed(pause_start_, std::chrono::steady_clock::now());
  spdlog::get(LOGGER)->info("Set pause off {}", time_in_pause_);
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
  ws_server_->SendMessage(username, "set_msg", std::make_shared<Msg>((success) ? "Success!" : "Failed!"));
}

void ServerGame::m_SendAudioMap(std::shared_ptr<Data> data) {
  spdlog::get(LOGGER)->info("ServerGame::InitializeGame: initializing with user: {} {}", host_, mode_);
  host_ = data->u();
  bool send_song = false;
  if (data->same_device()) {
    audio_.set_source_path(data->map_path());
  }
  else {
    std::string path = base_path_ + "/data/user-files/" + data->map_path();
    audio_.set_source_path(path);
    if (std::filesystem::exists(path)) {
      audio_data_ = utils::GetMedia(path);
      audio_file_name_ = data->audio_file_name();
    }
    else 
      send_song = true;
  }
  ws_server_->SendMessage(data->u(), "send_audio_info", std::make_shared<SendAudioInfo>(send_song));
}

void ServerGame::m_SendSong(std::shared_ptr<Data> data) {
  // Create initial data
  std::shared_ptr<Data> audio_data = std::make_shared<AudioTransferDataNew>(host_, audio_file_name_);
  std::map<int, std::string> contents;
  utils::SplitLargeData(contents, audio_data_, pow(2, 12));
  spdlog::get(LOGGER)->info("Made {} parts of {} bits data", contents.size(), audio_data_.size());
  audio_data->set_parts(contents.size()-1);
  for (const auto& it : contents) {
    audio_data->set_part(it.first);
    audio_data->set_content(it.second);
    try {
      ws_server_->SendMessage(data->u(), "send_audio_data", audio_data);
    } catch(...) { return; }
  }
  return;
}

void ServerGame::m_InitializeGame(std::shared_ptr<Data> data) {
  spdlog::get(LOGGER)->info("ServerGame::InitializeGame: initializing with user: {} {}", host_, mode_);

  // Get and analyze main audio-file (used for map and in SP for AI).
  std::string map_name = data->map_name();
  ws_server_->SendMessage(host_, "preparing", std::make_shared<Data>());
  audio_.Analyze();
  if (map_name.size() > 10) 
    map_name = map_name.substr(0, 10);

  // Get and analyze audio-files for AIs (OBSERVER-mode).
  std::vector<Audio*> audios; 
  if (data->ai_audio_data().size() > 0) {
    for (const auto& it : data->ai_audio_data()) {
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
    ws_server_->SendMessage(host_, "print_msg", std::make_shared<Msg>("Wainting for players..."));
  }
}

void ServerGame::PlayerResigned(std::string username) {
  if (players_.count(username) > 0 && players_.at(username) != nullptr && dead_players_.count(username) == 0) {
    players_.at(username)->set_lost(true);
    SendMessageToAllPlayers(Command("set_msg", std::make_shared<Msg>(username + " resigned")), username);
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
  SendInitialData();

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
    std::string msg = "Game cannot be played with this song, as generated map is unplayable. "
      "It might work with a higher resolution. (dissonance -r)";
    SendMessageToAllPlayers(Command("print_msg", std::make_shared<Msg>(msg)));
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
        // If multi player, inform other players.
        std::shared_ptr<Data> data = std::make_shared<GameEnd>("YOU LOST - times up");
        // Add statistics of all players to command.
        for (const auto& it : players_) 
          data->AddStatistics(it.second->GetFinalStatistics(it.first));
        SendMessageToAllPlayers(Command("game_end", data));
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
            std::vector<FieldPosition> new_units;
            for (const auto& pos : positions) 
              new_units.push_back(FieldPosition(pos, LOOPHOLE, it.second->color()));
            ws_server_->SendMessage(it.first, "set_units", std::make_shared<Units>(new_units));
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
      SendUpdate(1-(static_cast<float>(data_per_beat.size()) / audio_.analysed_data().data_per_beat_.size()));
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

std::map<position_t, std::pair<std::string, short>> ServerGame::GetAndUpdatePotentials() {
  std::map<position_t, std::pair<std::string, short>> potential_per_pos;
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

std::shared_ptr<Update> ServerGame::CreateBaseUpdate(float audio_played) {
  auto updated_potentials = GetAndUpdatePotentials();
  // Create player agnostic transfer-data
  std::map<std::string, std::pair<std::string, int>> players_status;
  std::map<position_t, int> new_dead_neurons;
  for (const auto& it : players_) {
    players_status[it.first] = {it.second->GetNucleusLive(), it.second->color()};
    for (const auto& it : it.second->new_dead_neurons())
      new_dead_neurons[it.first]= it.second;
  }
  return std::make_shared<Update>(players_status, updated_potentials, new_dead_neurons, audio_played);
}

void ServerGame::SendInitialData() {
  // Add player-agnostic data.
  auto update = CreateBaseUpdate(0);
  std::shared_ptr<Init> init = std::make_shared<Init>(update, field_->Export(players_), 
      field_->GraphPositions(), players_.begin()->second->technologies());
  // Add player-specific data
  for (const auto& it : human_players_) {
    init->set_macro(it.second->macro());
    init->update()->set_resources(it.second->t_resources());
    init->update()->set_build_options(it.second->GetBuildingOptions());
    init->update()->set_synapse_options(it.second->GetSynapseOptions());
    ws_server_->SendMessage(it.first, "init_game", init);
  }
  // Send data to all observers.
  for (const auto& it : observers_)
    ws_server_->SendMessage(it, "init_game", init);
  // Send all new neurons to obersers.
  SendNeuronsToObservers();
}

void ServerGame::SendUpdate(float audio_played) {
  auto update = CreateBaseUpdate(audio_played);
  for (const auto& it : human_players_) {
    update->set_resources(it.second->t_resources());
    update->set_build_options(it.second->GetBuildingOptions());
    update->set_synapse_options(it.second->GetSynapseOptions());
    ws_server_->SendMessage(it.first, "update_game", update);
  }
  // Send data to all observers.
  for (const auto& it : observers_)
    ws_server_->SendMessage(it, "update", update);
  // Send all new neurons to obersers.
  SendNeuronsToObservers();
}

void ServerGame::HandlePlayersLost() {
  if (status_ == CLOSING)
    return;
  std::shared_ptr<Data> data = std::make_shared<GameEnd>("YOU LOST");
  for (const auto& it : players_) 
    data->AddStatistics(it.second->GetFinalStatistics(it.first));
  // Check if new players have lost.
  for (const auto& it : players_) {
    if (it.second->HasLost() && dead_players_.count(it.first) == 0) {
      dead_players_.insert(it.first);
      // Send message if not AI.
      if (!IsAi(it.first) && ws_server_)
        ws_server_->SendMessage(it.first, "game_end", data);
    }
  }
  // If all but one players have one:
  if (dead_players_.size() == players_.size()-1) {
    // find only player who is not dead
    for (const auto& it : players_) {
      if (dead_players_.count(it.first) == 0) {
        data->set_msg(it.first + " WON"); // message set to contain name of winning player.
        // If not AI, send message.
        if (!IsAi(it.first) && ws_server_)
          ws_server_->SendMessage(it.first, "game_end", data);
      }
    }
    // Also inform all obersers.
    for (const auto& it : observers_) 
      ws_server_->SendMessage(it, "game_end", data); // message is the same as before
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
            ws_server_->SendMessage(it.first, "set_units", 
                std::make_shared<Units>(enemy->GetAllNeuronsInRange(nucleus)));
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
  std::vector<FieldPosition> units;
  for (const auto& it : players_) {
    for (const auto& neuron : it.second->new_neurons()) 
      units.push_back(FieldPosition(neuron.first, neuron.second, it.second->color())); 
  }
  for (const auto& it : observers_)
    ws_server_->SendMessage(it, "set_units", std::make_shared<Units>(units));
}

void ServerGame::SendMessageToAllPlayers(Command cmd, std::string ignore_username) {
  spdlog::get(LOGGER)->debug("ServerGame::SendMessageToAllPlayers: num human players: {}", human_players_.size());
  for (const auto& it : human_players_) {
    if (ignore_username == "" || it.first != ignore_username)
      ws_server_->SendMessage(it.first, cmd);
  }
}

void ServerGame::IpspSwallow(position_t ipsp_pos, Player* player, std::vector<Player*> enemies) {
  std::string ipsp_id = player->GetPotentialIdIfPotential(ipsp_pos, IPSP);
  for (const auto& enemy : enemies) {
    std::string id = enemy->GetPotentialIdIfPotential(ipsp_pos);
    if (id.find("epsp") != std::string::npos || id.find("macro_1") != std::string::npos) {
      player->NeutralizePotential(ipsp_id, -1); // increase potential by one
      if (enemy->NeutralizePotential(id, 1)) // decrease potential by one
        player->statistics()->AddEpspSwallowed();
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

Player* ServerGame::GetPlayer(std::string username) {
  if (players_.count(username) == 0)
    throw "ServerGame::GetPlayer: player not found!" + username;
  if (players_.at(username) == nullptr)
    throw "ServerGame::GetPlayer: player(" + username + ") is null~";
  return players_.at(username);
}
