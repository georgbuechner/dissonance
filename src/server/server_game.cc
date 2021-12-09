#include "server/server_game.h"
#include "audio/audio.h"
#include "constants/codes.h"
#include "nlohmann/json_fwd.hpp"
#include "objects/transfer.h"
#include "player/player.h"
#include "server/websocket_server.h"
#include <string>
#include <thread>

ServerGame::ServerGame(int lines, int cols, int mode, std::string base_path, WebsocketServer* srv, 
    std::string usr1, std::string usr2) : audio_(base_path), ws_server_(srv), usr1_id_(usr1), 
    usr2_id_(usr2), status_(0), mode_(mode), lines_(lines), cols_(cols) {}

nlohmann::json ServerGame::HandleInput(std::string command, std::string player, nlohmann::json data) {

  nlohmann::json response;

  if (command == "analyse_audio") {
    response = InitializeGame(data);
    spdlog::get(LOGGER)->info("returning response: {}", response.dump());
    spdlog::get(LOGGER)->flush();
  }
  else if (command == "add_iron") {
    if (player_one_->DistributeIron(data["resource"]))
      response = DistributeIron(player_one_, {false, "Selected!"});
    else 
      response = DistributeIron(player_one_, {true, "Not enough iron!"});
  }
  else if (command == "remove_iron") {
    if (player_one_->RemoveIron(data["resource"]))
      response = DistributeIron(player_one_, {false, "Selected!"});
    else 
      response = DistributeIron(player_one_, {true, "Not enough iron!"});
  }

  return response;
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
  status_ = 1;
  spdlog::get(LOGGER)->info("Game::InitializeGame: main events setup.");
  spdlog::get(LOGGER)->flush();

  return DistributeIron(player_one_);
}

void ServerGame::Thread_RenderField() {
  spdlog::get(LOGGER)->debug("Game::RenderField: started: {}", usr1_id_);
  spdlog::get(LOGGER)->flush();
  auto audio_start_time = std::chrono::steady_clock::now();
  auto analysed_data = audio_.analysed_data();
  std::list<AudioDataTimePoint> data_per_beat = analysed_data.data_per_beat_;

  auto last_update = std::chrono::steady_clock::now();
  auto last_resource_player_one = std::chrono::steady_clock::now();
  auto last_resource_player_two = std::chrono::steady_clock::now();

  double ki_resource_update_frequency = data_per_beat.front().bpm_;
  double player_resource_update_freqeuncy = data_per_beat.front().bpm_;
  double render_frequency = 40;

  auto pause_start_time = std::chrono::steady_clock::now();
  double time_in_pause = 0;
  bool pause_started = false;

  bool off_notes = false;
 
  while (!game_over_) {
    auto cur_time = std::chrono::steady_clock::now();

    if (pause_) {
      if (!pause_started) {
        pause_start_time = std::chrono::steady_clock::now();
        pause_started = true;
      }
      continue;
    }
    else if (pause_started) {
      time_in_pause += utils::GetElapsed(pause_start_time, cur_time);
      pause_started = false;
    }


    // Analyze audio data.
    auto elapsed = utils::GetElapsed(audio_start_time, cur_time)-time_in_pause;
    auto data_at_beat = data_per_beat.front();
    if (elapsed >= data_at_beat.time_) {
      spdlog::get(LOGGER)->debug("Game::RenderField: next data_at_beat");
      spdlog::get(LOGGER)->flush();

      render_frequency = 60000.0/(data_at_beat.bpm_*16);
      ki_resource_update_frequency = (60000.0/data_at_beat.bpm_); //*(data_at_beat.level_/50.0);
      player_resource_update_freqeuncy = 60000.0/(static_cast<double>(data_at_beat.bpm_)/2);
    
      off_notes = audio_.MoreOffNotes(data_at_beat);
      data_per_beat.pop_front();

      Transfer transfer;
      transfer.set_players({{usr1_id_, player_one_->GetNucleusLive()}, {"AI", player_two_->GetNucleusLive()}});
      transfer.set_field(field_->ToJson(player_one_, player_two_));
      nlohmann::json resp = {{"command", "print_field"}, {"data", nlohmann::json()}};
      if (mode_ == SINGLE_PLAYER) {
        transfer.set_resources(player_one_->t_resources());
        transfer.set_technologies(player_one_->t_technologies());
        resp["data"] = transfer.json();
        ws_server_->SendMessage(ws_server_->GetConnectionIdByUsername(usr1_id_), resp.dump());
      }
      if (mode_ == MULTI_PLAYER) {
        transfer.set_resources(player_one_->t_resources());
        transfer.set_technologies(player_one_->t_technologies());
        resp["data"] = transfer.json();
        ws_server_->SendMessage(ws_server_->GetConnectionIdByUsername(usr1_id_), resp.dump());
        transfer.set_resources(player_two_->t_resources());
        transfer.set_technologies(player_two_->t_technologies());
        resp["data"] = transfer.json();
        ws_server_->SendMessage(ws_server_->GetConnectionIdByUsername(usr2_id_), resp.dump());
      }
    }

    if (player_two_->HasLost() || player_one_->HasLost() || data_per_beat.size() == 0) {
      game_over_ = true;
      audio_.Stop();
      break;
    }
    if (resigned_) {
      game_over_ = true;
      audio_.Stop();
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
}

void ServerGame::Thread_Ai() {
  spdlog::get(LOGGER)->debug("Game::HandleActions: started");
  spdlog::get(LOGGER)->flush();
  auto audio_start_time = std::chrono::steady_clock::now();
  auto analysed_data = audio_.analysed_data();
  std::list<AudioDataTimePoint> data_per_beat = analysed_data.data_per_beat_;

  auto pause_start_time = std::chrono::steady_clock::now();
  double time_in_pause = 0;
  bool pause_started = false;

  // Handle building neurons and potentials.
  while(!game_over_) {
    auto cur_time = std::chrono::steady_clock::now(); 

    if (pause_) {
      if (!pause_started) {
        pause_start_time = std::chrono::steady_clock::now();
        pause_started = true;
      }
      continue;
    }
    else if (pause_started) {
      time_in_pause += utils::GetElapsed(pause_start_time, cur_time);
      pause_started = false;
    }

    // Analyze audio data.
    auto elapsed = utils::GetElapsed(audio_start_time, cur_time)-time_in_pause;
    if (data_per_beat.size() == 0)
      continue;
    auto data_at_beat = data_per_beat.front();
    if (elapsed >= data_at_beat.time_) {
      player_two_->DoAction(data_at_beat);
      player_two_->set_last_time_point(data_at_beat);
      data_per_beat.pop_front();
    }
  }
}


nlohmann::json ServerGame::DistributeIron(Player* player, std::pair<bool, std::string> error) {
  nlohmann::json data;
  data["help"] = "Iron (FE): " + player->resources().at(IRON).Print();
  data["resources"] = nlohmann::json();
  for (const auto& it : player->resources()) {
    nlohmann::json resource = {{"active", it.second.Active()}, {"iron", it.second.distributed_iron()}};
    data["resources"][it.first] = resource;
  }
  data["error"] = error.first;
  data["error_msg"] = error.second;
  return {{"command", "distribute_iron"}, {"data", data}};
}
