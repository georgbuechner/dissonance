#include "server/server_game.h"
#include "audio/audio.h"
#include "constants/codes.h"
#include "nlohmann/json_fwd.hpp"
#include "objects/transfer.h"
#include "player/player.h"
#include "server/websocket_server.h"
#include "share/eventmanager.h"
#include "spdlog/spdlog.h"
#include <mutex>
#include <string>
#include <thread>
#include <unistd.h>

ServerGame::ServerGame(int lines, int cols, int mode, std::string base_path, WebsocketServer* srv, 
    std::string usr1, std::string usr2) : audio_(base_path), ws_server_(srv), 
    usr1_id_(usr1), usr2_id_(usr2), status_(WAITING), mode_(mode), lines_(lines), cols_(cols) {

  // Initialize eventmanager.
  eventmanager_.AddHandler("analyse_audio", &ServerGame::m_AnalyzeAudio);
  eventmanager_.AddHandler("add_iron", &ServerGame::m_AddIron);
  eventmanager_.AddHandler("remove_iron", &ServerGame::m_RemoveIron);
  eventmanager_.AddHandler("add_technology", &ServerGame::m_AddTechnology);
  eventmanager_.AddHandler("resign", &ServerGame::m_Resign);
}


int ServerGame::status() {
  return status_;
}

void ServerGame::set_status(int status) {
  std::unique_lock ul(mutex_status_);
  status_ = status;
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

void ServerGame::m_AnalyzeAudio(nlohmann::json& msg) {
  msg = InitializeGame(msg["data"]);
}

void ServerGame::m_AddIron(nlohmann::json& msg) {
  if (player_one_->DistributeIron(msg["data"]["resource"]))
    msg = {{"command", "set_msg"}, {"data", {{"msg", "Distribute iron: done!"}} }};
  else 
    msg = {{"command", "set_msg"}, {"data", {{"msg", "Distribute iron: not enough iron!"}} }};
}

void ServerGame::m_RemoveIron(nlohmann::json& msg) {
  if (player_one_->RemoveIron(msg["data"]["resource"]))
    msg = {{"command", "set_msg"}, {"data", {{"msg", "Remove iron: done!"}} }};
  else 
    msg = {{"command", "set_msg"}, {"data", {{"msg", "Remove iron: not enough iron!"}} }};
}

void ServerGame::m_AddTechnology(nlohmann::json& msg) {
  if (player_one_->AddTechnology(msg["data"]["technology"]))
    msg = {{"command", "set_msg"}, {"data", {{"msg", "Add technology: done!"}} }};
  else 
    msg = {{"command", "set_msg"}, {"data", {{"msg", "Add technology: probably not enough resources!"}} }};
}

void ServerGame::m_Resign(nlohmann::json& msg) {
  std::unique_lock ul(mutex_status_);
  status_ = CLOSING;
  ul.unlock();
  // If multi player, inform other player.
  if (mode_ == MULTI_PLAYER) {
    std::string user_id = (msg["username"] == usr1_id_) ? usr2_id_ : usr1_id_;
    nlohmann::json resp = {{"command", "game_end"}, {"data", {{"msg", "YOU WON - opponent resigned"}} }};
    ws_server_->SendMessage(user_id, resp.dump());
  }
  msg = nlohmann::json();
}

// handlers
nlohmann::json ServerGame::InitializeGame(nlohmann::json data) {
  spdlog::get(LOGGER)->info("ServerGame::InitializeGame: initializing with user: {}", usr1_id_);
  nlohmann::json response;

  std::string source_path = data["source_path"];
  spdlog::get(LOGGER)->info("Selected path: {}", source_path);
  audio_.set_source_path(source_path);
  audio_.Analyze();

  // Build field.
  RandomGenerator* ran_gen = new RandomGenerator(audio_.analysed_data(), &RandomGenerator::ran_note);
  RandomGenerator* map_1 = new RandomGenerator(audio_.analysed_data(), &RandomGenerator::ran_boolean_minor_interval);
  RandomGenerator* map_2 = new RandomGenerator(audio_.analysed_data(), &RandomGenerator::ran_level_peaks);
  position_t nucleus_pos_1;
  position_t nucleus_pos_2;
  std::map<int, position_t> resource_positions_1;
  std::map<int, position_t> resource_positions_2;
  int denceness = 0;
  bool setup = false;
  spdlog::get(LOGGER)->info("Game::Play: creating map {} {} ", setup, denceness);
  while (!setup && denceness < 5) {
    spdlog::get(LOGGER)->info("Game::Play: creating map try: {}", denceness);

    field_ = new Field(lines_, cols_, ran_gen);
    field_->AddHills(map_1, map_2, denceness++);
    int player_one_section = (int)audio_.analysed_data().average_bpm_%8+1;
    int player_two_section = (int)audio_.analysed_data().average_level_%8+1;
    if (player_one_section == player_two_section)
      player_two_section = (player_two_section+1)%8;
    nucleus_pos_1 = field_->AddNucleus(player_one_section);
    nucleus_pos_2 = field_->AddNucleus(player_two_section);
    try {
      field_->BuildGraph(nucleus_pos_1, nucleus_pos_2);
      setup = true;
    } catch (std::logic_error& e) {
      spdlog::get(LOGGER)->warn("Game::play: graph could not be build: {}", e.what());
      field_ = NULL;
      continue;
    }
    resource_positions_1 = field_->AddResources(nucleus_pos_1);
    resource_positions_2 = field_->AddResources(nucleus_pos_2);
  }
  if (!setup) {
    response["command"] = "print_msg";
    response["data"] = { {"msg", {"Game cannot be played with this song, as map is unplayable. "
        "It might work with a higher resolution. (dissonance -r)"}}};
    return response;
  }

  // Setup players.
  player_one_ = new Player(nucleus_pos_1, field_, ran_gen, resource_positions_1);
  player_two_ = new AudioKi(nucleus_pos_2, field_, &audio_, ran_gen, resource_positions_2);
  player_one_->set_enemy(player_two_);
  player_two_->set_enemy(player_one_);
  player_two_->SetUpTactics(true); 

    // Let player two distribute initial iron.
  player_two_->DistributeIron(Resources::OXYGEN);
  player_two_->DistributeIron(Resources::OXYGEN);
  player_two_->HandleIron(audio_.analysed_data().data_per_beat_.front());

  // Start to main threads
  status_ = SETTING_UP;
  if (mode_ == SINGLE_PLAYER) {
    std::thread ai([this]() { Thread_Ai(); });
    ai.detach();
  }
  std::thread update([this]() { Thread_RenderField(); });
  update.detach();
  return {{"command", "game_start"}, {"data", nlohmann::json()}};
}

void ServerGame::Thread_RenderField() {
  spdlog::get(LOGGER)->info("Game::Thread_RenderField: started: {}", usr1_id_);
  auto audio_start_time = std::chrono::steady_clock::now();
  auto analysed_data = audio_.analysed_data();
  std::list<AudioDataTimePoint> data_per_beat = analysed_data.data_per_beat_;

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

      // Create transfer data.
      Transfer transfer;
      transfer.set_players({{usr1_id_, player_one_->GetNucleusLive()}, {"AI", player_two_->GetNucleusLive()}});
      transfer.set_field(field_->ToJson(player_one_, player_two_));
      nlohmann::json resp = {{"command", "print_field"}, {"data", nlohmann::json()}};
      // Send data to single player
      if (mode_ == SINGLE_PLAYER) {
        transfer.set_resources(player_one_->t_resources());
        transfer.set_technologies(player_one_->t_technologies());
        resp["data"] = transfer.json();
        ws_server_->SendMessage(usr1_id_, resp.dump());
      }
      // Send data to multiple players 
      else if (mode_ == MULTI_PLAYER) {
        transfer.set_resources(player_one_->t_resources());
        transfer.set_technologies(player_one_->t_technologies());
        resp["data"] = transfer.json();
        ws_server_->SendMessage(usr1_id_, resp.dump());
        transfer.set_resources(player_two_->t_resources());
        transfer.set_technologies(player_two_->t_technologies());
        resp["data"] = transfer.json();
        ws_server_->SendMessage(usr2_id_, resp.dump());
      }
    }

    // Handle game over
    if (player_two_->HasLost() || player_one_->HasLost() || data_per_beat.size() == 0) {
      nlohmann::json resp = {{"command", "game_end"}, {"data", nlohmann::json()}};
      resp["data"]["msg"] = (player_two_->HasLost()) ? "YOU WON" 
        : (data_per_beat.size() == 0) ? "YOU LOST - time up" : "YOU LOST";
      ws_server_->SendMessage(usr1_id_, resp.dump());
      if (mode_ == MULTI_PLAYER) {
        resp["data"]["msg"] = (player_one_->HasLost()) ? "YOU WON" 
          : (data_per_beat.size() == 0) ? "YOU LOST - time up" : "YOU LOST";
        ws_server_->SendMessage(usr1_id_, resp.dump());
      }
      {
        std::unique_lock ul(mutex_status_);
        status_ = CLOSING;
      }
      break;
    }
  
    // Increase resources.
    if (utils::GetElapsed(last_resource_player_one, cur_time) > player_resource_update_freqeuncy) {
      player_one_->IncreaseResources(off_notes);
      last_resource_player_one = cur_time;
    }
    if (utils::GetElapsed(last_resource_player_two, cur_time) > ki_resource_update_frequency) {
      player_two_->IncreaseResources(off_notes);
      last_resource_player_two = cur_time;
    }

    // Move potential
    if (utils::GetElapsed(last_update, cur_time) > render_frequency) {
      // Move player soldiers and check if enemy den's lp is down to 0.
      player_one_->MovePotential(player_two_);
      player_two_->MovePotential(player_one_);

      // Remove enemy soldiers in renage of defence towers.
      player_one_->HandleDef(player_two_);
      player_two_->HandleDef(player_one_);

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
      player_two_->DoAction(data_at_beat);
      player_two_->set_last_time_point(data_at_beat);
      data_per_beat.pop_front();
    }
  }
  spdlog::get(LOGGER)->info("Game::Thread_Ai: ended");
}
