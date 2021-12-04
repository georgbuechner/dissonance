#include "server/server_game.h"
#include "audio/audio.h"
#include "nlohmann/json_fwd.hpp"
#include "player/player.h"
#include <string>

ServerGame::ServerGame(int lines, int cols, int mode, std::string base_path) 
  : audio_(base_path), mode_(mode), lines_(lines), cols_(cols) {}

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

  return DistributeIron(player_one_);
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
