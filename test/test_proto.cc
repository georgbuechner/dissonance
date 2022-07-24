#include <catch2/catch.hpp>
#include <cstddef>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "curses.h"
#include "share/constants/codes.h"
#include "share/defines.h"
#include "share/shemes/commands.h"
#include "share/shemes/data.h"
#include "share/tools/utils/utils.h"

TEST_CASE("Test creating simple dto", "[msgpack]") {
  std::string command = "preparing";
  Command cmd(command);
  REQUIRE(cmd.command() == command);
  auto payload = cmd.bytes();

  // convert to binaryt
  Command cmd_from_bytes(payload.c_str(), payload.size());
  REQUIRE(cmd_from_bytes.command() == command);
}

TEST_CASE("Test creating msg dto", "[msgpack]") {
  std::string command = "print_msg";
  std::string msg = "Haaaallooooo ich bin JanJan!";

  SECTION ("Test create msg-command", "[msgpack]") {
    std::shared_ptr<Data> data = std::make_shared<Msg>(msg);
    Command cmd(command, data);
    auto payload = cmd.bytes();
    REQUIRE(cmd.command() == command);
    REQUIRE(cmd.data()->msg() == msg);

    Command cmd_from_bytes(payload.c_str(), payload.size());
    REQUIRE(cmd_from_bytes.command() == command);
    REQUIRE(cmd_from_bytes.data()->msg() == msg);
  }
}

TEST_CASE("Test creating setup_new_game-dto", "[msgpack]") {
  std::string command = "setup_new_game";
  unsigned short lines = 100;
  unsigned short cols = 50;

  SECTION ("Test create setup_new_game-command for single-player", "[msgpack]") {
    unsigned short mode = SINGLE_PLAYER;
    std::shared_ptr<Data> data = std::make_shared<InitNewGame>(mode, lines, cols, false);
    Command cmd(command, data);
    REQUIRE(cmd.command() == command);
    REQUIRE(cmd.data()->mode() == mode);
    REQUIRE(cmd.data()->lines() == lines);
    REQUIRE(cmd.data()->cols() == cols);
    REQUIRE(cmd.data()->num_players() == 0);
    REQUIRE(cmd.data()->game_id() == "");
    auto payload = cmd.bytes();

    Command cmd_from_bytes(payload.c_str(), payload.size());
    REQUIRE(cmd_from_bytes.command() == command);
    REQUIRE(cmd_from_bytes.data()->mode() == mode);
    REQUIRE(cmd_from_bytes.data()->lines() == lines);
    REQUIRE(cmd_from_bytes.data()->cols() == cols);
    REQUIRE(cmd_from_bytes.data()->num_players() == 0);
    REQUIRE(cmd_from_bytes.data()->game_id() == "");
  }

  SECTION ("Test create setup_new_game-command for multi-player (host)", "[msgpack]") {
    unsigned short num_players = 2;
    unsigned short mode = MULTI_PLAYER;
    std::shared_ptr<Data> data = std::make_shared<InitNewGame>(mode, lines, cols, false);
    data->set_num_players(num_players);
    Command cmd(command, data);
    REQUIRE(cmd.command() == command);
    REQUIRE(cmd.data()->mode() == mode);
    REQUIRE(cmd.data()->lines() == lines);
    REQUIRE(cmd.data()->cols() == cols);
    REQUIRE(cmd.data()->num_players() == num_players);
    REQUIRE(cmd.data()->game_id() == "");
    auto payload = cmd.bytes();

    Command cmd_from_bytes(payload.c_str(), payload.size());
    REQUIRE(cmd_from_bytes.command() == command);
    REQUIRE(cmd_from_bytes.data()->mode() == mode);
    REQUIRE(cmd_from_bytes.data()->lines() == lines);
    REQUIRE(cmd_from_bytes.data()->cols() == cols);
    REQUIRE(cmd_from_bytes.data()->num_players() == num_players);
    REQUIRE(cmd_from_bytes.data()->game_id() == "");
  }

  SECTION ("Test create setup_new_game-command for multi-player (client)", "[msgpack]") {
    std::string game_id = "fux_game";
    unsigned short mode = MULTI_PLAYER_CLIENT;
    std::shared_ptr<Data> data = std::make_shared<InitNewGame>(mode, lines, cols, false);
    data->set_game_id(game_id);
    Command cmd(command, data);
    REQUIRE(cmd.command() == command);
    REQUIRE(cmd.data()->mode() == mode);
    REQUIRE(cmd.data()->lines() == lines);
    REQUIRE(cmd.data()->cols() == cols);
    REQUIRE(cmd.data()->num_players() == 0);
    REQUIRE(cmd.data()->game_id() == game_id);
    auto payload = cmd.bytes();

    Command cmd_from_bytes(payload.c_str(), payload.size());
    REQUIRE(cmd_from_bytes.command() == command);
    REQUIRE(cmd_from_bytes.data()->mode() == mode);
    REQUIRE(cmd_from_bytes.data()->lines() == lines);
    REQUIRE(cmd_from_bytes.data()->cols() == cols);
    REQUIRE(cmd_from_bytes.data()->num_players() == 0);
    REQUIRE(cmd_from_bytes.data()->game_id() == game_id);
  }
}

TEST_CASE("Test creating set_unit dto", "[msgpack]") {
  std::string command = "set_unit";
  position_t pos = {74, 89};
  int unit = 1;
  int color = 13;

  SECTION ("Test create set_unit-command", "[msgpack]") {
    std::shared_ptr<Data> data = std::make_shared<FieldPosition>(pos, unit, color);
    Command cmd(command, data);
    REQUIRE(cmd.command() == command);
    REQUIRE(cmd.data()->pos() == pos);
    REQUIRE(cmd.data()->unit() == unit);
    REQUIRE(cmd.data()->color() == color);
    auto payload = cmd.bytes();

    Command cmd_from_bytes(payload.c_str(), payload.size());
    REQUIRE(cmd_from_bytes.command() == command);
    REQUIRE(cmd_from_bytes.data()->pos() == pos);
    REQUIRE(cmd_from_bytes.data()->unit() == unit);
    REQUIRE(cmd_from_bytes.data()->color() == color);
  }
}

TEST_CASE("Test creating set_units dto", "[msgpack]") {
  std::string command = "set_units";

  SECTION ("Test create set_units-command with two units", "[msgpack]") {
    std::vector<FieldPosition> units = {FieldPosition({1,1}, 1, 12), FieldPosition({100,50}, 2, 13)};
    std::shared_ptr<Data> data = std::make_shared<Units>(units);
    Command cmd(command, data);
    auto payload = cmd.bytes();
    REQUIRE(cmd.command() == command);
    REQUIRE(cmd.data()->units().size() == units.size());

    Command cmd_from_bytes(payload.c_str(), payload.size());
    REQUIRE(cmd_from_bytes.command() == command);
    REQUIRE(cmd_from_bytes.data()->units().size() == units.size());
  }
}

TEST_CASE("Test creating update_game dto", "[msgpack]") {
  std::string command = "update_game";
  std::map<std::string, std::pair<std::string, int>> players = {{"fux", {"0/9", 1}}, {"georg", {"5/9", 2}}};
  std::map<position_t, int> new_dead_neurons = {{{99, 99}, 3}, {{23, 7}, 2}};
  std::map<position_t, std::pair<std::string, short>> potentials = {};
  float audio_played = 0.7;
  std::map<int, Data::Resource> resources = {
    {1, Data::Resource(34.5, 23.4, 100, 2, true)}, 
    {2, Data::Resource(43.5, 32.4, 120, 0, false)}
  };
  std::vector<bool> build_options = { true, true, false, false, false};
  std::vector<bool> synapse_options = { false, false, true, true, true, false};

  SECTION ("Test create update_game-command", "[msgpack]") {
    std::shared_ptr<Data> data = std::make_shared<Update>(players, potentials, new_dead_neurons, audio_played);
    data->set_resources(resources);
    data->set_build_options(build_options);
    data->set_synapse_options(synapse_options);
    Command cmd(command, data);
    REQUIRE(cmd.command() == command);
    REQUIRE(cmd.data()->players() == players);
    REQUIRE(cmd.data()->new_dead_neurons() == new_dead_neurons);
    REQUIRE(cmd.data()->audio_played() == audio_played);
    for (const auto& it : resources) {
      REQUIRE(cmd.data()->resources().at(it.first).value_ == it.second.value_);
      REQUIRE(cmd.data()->resources().at(it.first).active_ == it.second.active_);
    }
    REQUIRE(cmd.data()->build_options() == build_options);
    REQUIRE(cmd.data()->synapse_options() == synapse_options);
    auto payload = cmd.bytes();

    Command cmd_from_bytes(payload.c_str(), payload.size());
    REQUIRE(cmd_from_bytes.command() == command);
    REQUIRE(cmd_from_bytes.data()->players() == players);
    REQUIRE(cmd_from_bytes.data()->new_dead_neurons() == new_dead_neurons);
    REQUIRE(cmd_from_bytes.data()->audio_played() == audio_played);
    for (const auto& it : resources) {
      REQUIRE(cmd_from_bytes.data()->resources().at(it.first).value_ == it.second.value_);
      REQUIRE(cmd_from_bytes.data()->resources().at(it.first).active_ == it.second.active_);
    }
    REQUIRE(cmd_from_bytes.data()->build_options() == build_options);
    REQUIRE(cmd_from_bytes.data()->synapse_options() == synapse_options);
  }
}

TEST_CASE("Test creating init_game-dto update", "[msgpack]") {
  std::string command = "init_game";

  // Create update-data
  std::map<std::string, std::pair<std::string, int>> players = {{"fux", {"0/9", 1}}, {"georg", {"5/9", 2}}};
  std::map<position_t, std::pair<std::string, short>> potentials = {{{99, 99}, {"10", EPSP}}, {{23, 7}, {"c", IPSP}}};
  std::map<position_t, int> new_dead_neurons = {{{99, 99}, 3}, {{23, 7}, 2}};
  float audio_played = 0.7;
  std::map<int, Data::Resource> resources = {
    {1, Data::Resource(34.5, 23.4, 100, 2, true)}, 
    {2, Data::Resource(43.5, 32.4, 120, 0, false)}
  };
  std::vector<bool> build_options = { true, true, false, false, false};
  std::vector<bool> synapse_options = { false, false, true, true, true, false};
  std::shared_ptr<Update> update = std::make_shared<Update>(players, potentials, new_dead_neurons, audio_played);

  // Create field-data
  std::vector<std::vector<Data::Symbol>> field = {
    {Data::Symbol({"S", 0}), Data::Symbol({"S", 0}), Data::Symbol({"S", 1}), Data::Symbol({"S", 0})},
    {Data::Symbol({"S", 0}), Data::Symbol({"S", 0}), Data::Symbol({"S", 0}), Data::Symbol({"S", 1})},
    {Data::Symbol({"T", 2}), Data::Symbol({"S", 1}), Data::Symbol({"S", 0}), Data::Symbol({"S", 1})},
    {Data::Symbol({"X", 0}), Data::Symbol({"Y", 0}), Data::Symbol({"S", 1}), Data::Symbol({"S", 0})},
  };
  std::vector<position_t> graph_positions = {{2,3}, {99,3}, {44, 23}, {10, 12}};
  std::map<int, tech_of_t> technologies = {{1, {0,3}}, {2, {0, 3}}, {3, {0, 3}}, {4, {1,3}}};
  int macro = 1;

  SECTION ("Test create update_game-command", "[msgpack]") {
    std::shared_ptr<Data> data = std::make_shared<Init>(update, field, graph_positions, technologies);
    // set player-specific data
    data->set_macro(macro);
    data->update()->set_resources(resources);
    data->update()->set_build_options(build_options);
    data->update()->set_synapse_options(synapse_options);
    
    Command cmd(command, data);
    auto payload = cmd.bytes();

    REQUIRE(cmd.command() == command);
    REQUIRE(cmd.data()->field().size() == field.size());
    REQUIRE(cmd.data()->graph_positions() == graph_positions);
    REQUIRE(cmd.data()->technologies() == technologies);
    REQUIRE(cmd.data()->macro() == macro);
    REQUIRE(cmd.data()->update()->players() == players);
    REQUIRE(cmd.data()->update()->potentials() == potentials);
    REQUIRE(cmd.data()->update()->new_dead_neurons() == new_dead_neurons);
    REQUIRE(cmd.data()->update()->audio_played() == audio_played);
    for (const auto& it : resources) {
      REQUIRE(cmd.data()->update()->resources().at(it.first).value_ == it.second.value_);
      REQUIRE(cmd.data()->update()->resources().at(it.first).active_ == it.second.active_);
    }
    REQUIRE(cmd.data()->update()->build_options() == build_options);
    REQUIRE(cmd.data()->update()->synapse_options() == synapse_options);

    Command cmd_from_bytes(payload.c_str(), payload.size());

    REQUIRE(cmd_from_bytes.command() == command);
    REQUIRE(cmd_from_bytes.data()->update()->players() == players);
    REQUIRE(cmd_from_bytes.data()->update()->potentials() == potentials);
    REQUIRE(cmd_from_bytes.data()->update()->new_dead_neurons() == new_dead_neurons);
    REQUIRE(cmd_from_bytes.data()->update()->audio_played() == audio_played);
    for (const auto& it : resources) {
      REQUIRE(cmd_from_bytes.data()->update()->resources().at(it.first).value_ == it.second.value_);
      REQUIRE(cmd_from_bytes.data()->update()->resources().at(it.first).active_ == it.second.active_);
    }
    REQUIRE(cmd_from_bytes.data()->update()->build_options() == build_options);
    REQUIRE(cmd_from_bytes.data()->update()->synapse_options() == synapse_options);
  }
}

TEST_CASE("Test creating update_lobby-dto", "[msgpack]") {
  std::string command = "update_lobby";
  std::shared_ptr<Data> data = std::make_shared<Lobby>();
  data->AddEntry("fux-game", 4, 2, "here-my-call.mp3");
  data->AddEntry("georgs-game", 2, 0, "black sands.wav");

  Command cmd(command, data);
  REQUIRE(cmd.command() == command);
  REQUIRE(cmd.data()->lobby().size() == 2);
  auto payload = cmd.bytes();

  Command cmd_from_bytes(payload.c_str(), payload.size());
  REQUIRE(cmd_from_bytes.command() == command);
  REQUIRE(cmd_from_bytes.data()->lobby().size() == 2);
  for (unsigned int i=0; i<cmd.data()->lobby().size(); i++) {
    REQUIRE(cmd_from_bytes.data()->lobby()[i]._max_players == cmd.data()->lobby()[i]._max_players);
    REQUIRE(cmd_from_bytes.data()->lobby()[i]._cur_players == cmd.data()->lobby()[i]._cur_players);
    REQUIRE(cmd_from_bytes.data()->lobby()[i]._game_id == cmd.data()->lobby()[i]._game_id);
    REQUIRE(cmd_from_bytes.data()->lobby()[i]._audio_map_name == cmd.data()->lobby()[i]._audio_map_name);
  }
}

TEST_CASE("Test creating game_end-dto", "[msgpack]") {
  std::string command = "game_end";
  std::string msg = "YOU LOST";
  std::shared_ptr<Data> data= std::make_shared<GameEnd>(msg);
  // Create statistics
  std::shared_ptr<Statictics> statistics_1 = std::make_shared<Statictics>();
  statistics_1->set_color(1);
  for (unsigned int i=0; i<10; i++)
    statistics_1->AddEpspSwallowed();
  for (unsigned int i=0; i<100; i++)
    statistics_1->AddKillderPotential("epsp_");
  for (unsigned int i=0; i<15; i++)
    statistics_1->AddKillderPotential("ipsp_");
  for (unsigned int i=0; i<95; i++)
    statistics_1->AddLostPotential("epsp_");
  for (unsigned int i=0; i<5; i++)
    statistics_1->AddLostPotential("ipsp_");
  statistics_1->AddNewPotential(1);
  statistics_1->stats_resources_ref()[1] = {{"1", 3.5}, {"3", 7.6}};
  statistics_1->stats_resources_ref()[2] = {{"5", 1.5}, {"8", 3.6}};
  statistics_1->set_technologies({{1, {0,3}}, {2, {2,3}}});
  data->AddStatistics(statistics_1);

  std::shared_ptr<Statictics> statistics_2 = std::make_shared<Statictics>();
  statistics_2->set_color(1);
  for (unsigned int i=0; i<11; i++)
    statistics_2->AddEpspSwallowed();
  for (unsigned int i=0; i<120; i++)
    statistics_2->AddKillderPotential("epsp_");
  for (unsigned int i=0; i<13; i++)
    statistics_2->AddKillderPotential("ipsp_");
  for (unsigned int i=0; i<92; i++)
    statistics_2->AddLostPotential("epsp_");
  for (unsigned int i=0; i<6; i++)
    statistics_2->AddLostPotential("ipsp_");
  statistics_2->AddNewPotential(2);
  statistics_2->stats_resources_ref()[3] = {{"1", 3.5}, {"3", 7.6}};
  statistics_2->stats_resources_ref()[1] = {{"5", 1.5}, {"8", 3.6}};
  statistics_2->set_technologies({{1, {0,3}}, {2, {2,3}}});
  data->AddStatistics(statistics_2);

  Command cmd(command, data);
  REQUIRE(cmd.command() == command);
  REQUIRE(cmd.data()->msg() == msg);
  REQUIRE(cmd.data()->statistics()[0]->epsp_swallowed() == statistics_1->epsp_swallowed());
  REQUIRE(cmd.data()->statistics()[1]->epsp_swallowed() == statistics_2->epsp_swallowed());
  auto payload = cmd.bytes();

  Command cmd_from_bytes(payload.c_str(), payload.size());
  REQUIRE(cmd_from_bytes.command() == command);
  REQUIRE(cmd_from_bytes.data()->msg() == msg);
  REQUIRE(cmd_from_bytes.data()->statistics()[0]->epsp_swallowed() == statistics_1->epsp_swallowed());
  REQUIRE(cmd_from_bytes.data()->statistics()[1]->epsp_swallowed() == statistics_2->epsp_swallowed());
}

TEST_CASE("Test creating build_potential-dto", "[msgpack]") {
  std::string command = "build_potential";
  short unit = EPSP;
  short num = 1;
  position_t default_pos = {-1, -1};
  position_t synapse_pos = {3,4};
  std::vector<position_t> positions = { {3,4}, {5,6}, {0,0}};

  SECTION("Create initial data", "[msgpack]") {
    std::shared_ptr<Data> data = std::make_shared<BuildPotential>(unit, num);
    Command cmd(command, data);
    REQUIRE(cmd.command() == command);
    REQUIRE(cmd.data()->unit() == unit);
    REQUIRE(cmd.data()->num() == num);
    REQUIRE(cmd.data()->start_pos() == default_pos);
    REQUIRE(cmd.data()->positions().size() == 0);
    auto payload = cmd.bytes();

    Command cmd_from_bytes(payload.c_str(), payload.size());
    REQUIRE(cmd_from_bytes.command() == command);
    REQUIRE(cmd_from_bytes.data()->unit() == unit);
    REQUIRE(cmd_from_bytes.data()->num() == num);
    REQUIRE(cmd_from_bytes.data()->synapse_pos() == default_pos);
    REQUIRE(cmd_from_bytes.data()->positions().size() == 0);
  }

  SECTION("Create data with positions", "[msgpack]") {
    std::shared_ptr<Data> data = std::make_shared<BuildPotential>(unit, num);
    Command cmd(command, data);
    cmd.data()->set_positions(positions);
    REQUIRE(cmd.command() == command);
    REQUIRE(cmd.data()->unit() == unit);
    REQUIRE(cmd.data()->num() == num);
    REQUIRE(cmd.data()->synapse_pos() == default_pos);
    REQUIRE(cmd.data()->positions() == positions);
    auto payload = cmd.bytes();

    Command cmd_from_bytes(payload.c_str(), payload.size());
    REQUIRE(cmd_from_bytes.command() == command);
    REQUIRE(cmd_from_bytes.data()->unit() == unit);
    REQUIRE(cmd_from_bytes.data()->num() == num);
    REQUIRE(cmd_from_bytes.data()->synapse_pos() == default_pos);
    REQUIRE(cmd_from_bytes.data()->positions() == positions);
  }

  SECTION("Create data with positions and start-pos", "[msgpack]") {
    std::shared_ptr<Data> data = std::make_shared<BuildPotential>(unit, num);
    Command cmd(command, data);
    cmd.data()->set_positions(positions);
    cmd.data()->set_start_pos(synapse_pos);
    REQUIRE(cmd.command() == command);
    REQUIRE(cmd.data()->unit() == unit);
    REQUIRE(cmd.data()->num() == num);
    REQUIRE(cmd.data()->synapse_pos() == synapse_pos);
    REQUIRE(cmd.data()->positions() == positions);
    auto payload = cmd.bytes();

    Command cmd_from_bytes(payload.c_str(), payload.size());
    REQUIRE(cmd_from_bytes.command() == command);
    REQUIRE(cmd_from_bytes.data()->unit() == unit);
    REQUIRE(cmd_from_bytes.data()->num() == num);
    REQUIRE(cmd_from_bytes.data()->synapse_pos() == synapse_pos);
    REQUIRE(cmd_from_bytes.data()->positions() == positions);
  }

  SECTION("Create data with start-pos but without positions", "[msgpack]") {
    std::shared_ptr<Data> data = std::make_shared<BuildPotential>(unit, num);
    Command cmd(command, data);
    cmd.data()->set_start_pos(synapse_pos);
    REQUIRE(cmd.command() == command);
    REQUIRE(cmd.data()->unit() == unit);
    REQUIRE(cmd.data()->num() == num);
    REQUIRE(cmd.data()->synapse_pos() == synapse_pos);
    REQUIRE(cmd.data()->positions().size() == 0);
    auto payload = cmd.bytes();

    Command cmd_from_bytes(payload.c_str(), payload.size());
    REQUIRE(cmd_from_bytes.command() == command);
    REQUIRE(cmd_from_bytes.data()->unit() == unit);
    REQUIRE(cmd_from_bytes.data()->num() == num);
    REQUIRE(cmd_from_bytes.data()->synapse_pos() == synapse_pos);
    REQUIRE(cmd_from_bytes.data()->positions().size() == 0);
  }
}

TEST_CASE("Test creating build_neuron-dto", "[msgpack]") {
  std::string command = "build_neuron";
  short unit = EPSP;
  short range = 6;
  position_t default_pos = {-1, -1};
  position_t pos = {3,6};
  position_t start_pos = {3,4};
  std::vector<position_t> positions = { {3,4}, {5,6}, {0,0}};

  SECTION("Create initial build-neuron-data", "[msgpack]") {
    std::shared_ptr<Data> data = std::make_shared<BuildNeuron>(unit);
    Command cmd(command, data);
    REQUIRE(cmd.command() == command);
    REQUIRE(cmd.data()->unit() == unit);
    // unsert variables
    REQUIRE(cmd.data()->range() == 0);
    REQUIRE(cmd.data()->pos() == default_pos);
    REQUIRE(cmd.data()->start_pos() == default_pos);
    REQUIRE(cmd.data()->positions().size() == 0);
    auto payload = cmd.bytes();

    Command cmd_from_bytes(payload.c_str(), payload.size());
    REQUIRE(cmd_from_bytes.command() == command);
    REQUIRE(cmd_from_bytes.data()->unit() == unit);
    REQUIRE(cmd_from_bytes.data()->range() == 0);
    REQUIRE(cmd.data()->pos() == default_pos);
    REQUIRE(cmd_from_bytes.data()->start_pos() == default_pos);
    REQUIRE(cmd_from_bytes.data()->positions().size() == 0);
  }

  SECTION("Create build-neuron-data with positions", "[msgpack]") {
    std::shared_ptr<Data> data = std::make_shared<BuildNeuron>(unit);
    Command cmd(command, data);
    cmd.data()->set_positions(positions);
    cmd.data()->set_range(range);
    REQUIRE(cmd.command() == command);
    REQUIRE(cmd.data()->unit() == unit);
    REQUIRE(cmd.data()->range() == range);
    REQUIRE(cmd.data()->pos() == default_pos);
    REQUIRE(cmd.data()->start_pos() == default_pos);
    REQUIRE(cmd.data()->positions() == positions);
    auto payload = cmd.bytes();

    Command cmd_from_bytes(payload.c_str(), payload.size());
    REQUIRE(cmd_from_bytes.command() == command);
    REQUIRE(cmd_from_bytes.data()->unit() == unit);
    REQUIRE(cmd_from_bytes.data()->range() == range);
    REQUIRE(cmd_from_bytes.data()->pos() == default_pos);
    REQUIRE(cmd_from_bytes.data()->start_pos() == default_pos);
    REQUIRE(cmd_from_bytes.data()->positions() == positions);
  }

  SECTION("Create build-neuron-data with positions and start-pos", "[msgpack]") {
    std::shared_ptr<Data> data = std::make_shared<BuildNeuron>(unit);
    Command cmd(command, data);
    cmd.data()->set_range(range);
    cmd.data()->set_start_pos(start_pos);
    REQUIRE(cmd.command() == command);
    REQUIRE(cmd.data()->unit() == unit);
    REQUIRE(cmd.data()->range() == range);
    REQUIRE(cmd.data()->pos() == default_pos);
    REQUIRE(cmd.data()->start_pos() == start_pos);
    REQUIRE(cmd.data()->positions().size() == 0);
    auto payload = cmd.bytes();

    Command cmd_from_bytes(payload.c_str(), payload.size());
    REQUIRE(cmd_from_bytes.command() == command);
    REQUIRE(cmd_from_bytes.data()->unit() == unit);
    REQUIRE(cmd_from_bytes.data()->range() == range);
    REQUIRE(cmd_from_bytes.data()->pos() == default_pos);
    REQUIRE(cmd_from_bytes.data()->start_pos() == start_pos);
    REQUIRE(cmd_from_bytes.data()->positions().size() == 0);
  }

  SECTION("Create build-neuron-data with start-pos but without positions", "[msgpack]") {
    std::shared_ptr<Data> data = std::make_shared<BuildNeuron>(unit);
    Command cmd(command, data);
    cmd.data()->set_range(range);
    cmd.data()->set_start_pos(start_pos);
    cmd.data()->set_pos(pos);

    REQUIRE(cmd.command() == command);
    REQUIRE(cmd.data()->unit() == unit);
    REQUIRE(cmd.data()->range() == range);
    REQUIRE(cmd.data()->pos() == pos);
    REQUIRE(cmd.data()->start_pos() == start_pos);
    REQUIRE(cmd.data()->positions().size() == 0);
    auto payload = cmd.bytes();

    Command cmd_from_bytes(payload.c_str(), payload.size());
    REQUIRE(cmd_from_bytes.command() == command);
    REQUIRE(cmd_from_bytes.data()->unit() == unit);
    REQUIRE(cmd_from_bytes.data()->range() == range);
    REQUIRE(cmd.data()->pos() == pos);
    REQUIRE(cmd_from_bytes.data()->start_pos() == start_pos);
    REQUIRE(cmd_from_bytes.data()->positions().size() == 0);
  }
}

TEST_CASE("Test creating select-synapse-dto", "[msgpack]") {
  std::string command = "select_synapse";
  position_t default_pos = {-1, -1};
  position_t synapse_pos = {40, 40};
  std::vector<position_t> player_units = {{40, 40}, {50, 50}};

  SECTION ("Test create select_synapse-command without synapse_pos", "[msgpack]") {
    std::shared_ptr<Data> data = std::make_shared<SelectSynapse>(player_units);
    Command cmd(command, data);
    REQUIRE(cmd.command() == command);
    REQUIRE(cmd.data()->player_units() == player_units);
    REQUIRE(cmd.data()->synapse_pos() == default_pos);
    auto payload = cmd.bytes();

    Command cmd_from_bytes(payload.c_str(), payload.size());
    REQUIRE(cmd_from_bytes.command() == command);
    REQUIRE(cmd_from_bytes.data()->player_units() == player_units);
    REQUIRE(cmd_from_bytes.data()->synapse_pos() == default_pos);
  }

  SECTION ("Test create select_synapse-command with synapse_pos", "[msgpack]") {
    std::shared_ptr<Data> data = std::make_shared<SelectSynapse>(player_units);
    Command cmd(command, data);
    cmd.data()->set_synapse_pos(synapse_pos);
    REQUIRE(cmd.command() == command);
    REQUIRE(cmd.data()->player_units() == player_units);
    REQUIRE(cmd.data()->synapse_pos() == synapse_pos);
    auto payload = cmd.bytes();

    Command cmd_from_bytes(payload.c_str(), payload.size());
    REQUIRE(cmd_from_bytes.command() == command);
    REQUIRE(cmd_from_bytes.data()->player_units() == player_units);
    REQUIRE(cmd_from_bytes.data()->synapse_pos() == synapse_pos);
  }
}

TEST_CASE("Test creating set_wps-dto", "[msgpack]") {
  std::string command = "set_wps";
  position_t default_pos = {-1, -1};
  position_t synapse_pos = {40, 40};
  position_t way_point = {56, 12};
  std::vector<position_t> centered_positions = {{40, 40}, {50, 50}};
  std::vector<position_t> current_way = {{30, 31}, {30, 32}, {31, 32}, {32, 32}, {33, 33}, {34, 34}, {34, 35}};
  std::vector<position_t> current_waypoints = {{30, 31}, {33, 33}};
  std::string msg = "all availible way-points set!";
  int num = 3;

  SECTION ("Test create set_wps-command with only synapse_pos", "[msgpack]") {
    std::shared_ptr<Data> data = std::make_shared<SetWayPoints>(synapse_pos);
    Command cmd(command, data);
    REQUIRE(cmd.command() == command);
    REQUIRE(cmd.data()->centered_positions().size() == 0);
    REQUIRE(cmd.data()->current_way().size() == 0);
    REQUIRE(cmd.data()->current_waypoints().size() == 0);
    REQUIRE(cmd.data()->synapse_pos() == synapse_pos);
    REQUIRE(cmd.data()->way_point() == default_pos);
    REQUIRE(cmd.data()->msg() == "");
    REQUIRE(cmd.data()->num() == 1);
    auto payload = cmd.bytes();

    Command cmd_from_bytes(payload.c_str(), payload.size());
    REQUIRE(cmd_from_bytes.command() == command);
    REQUIRE(cmd_from_bytes.data()->centered_positions().size() == 0);
    REQUIRE(cmd_from_bytes.data()->current_way().size() == 0);
    REQUIRE(cmd_from_bytes.data()->current_waypoints().size() == 0);
    REQUIRE(cmd_from_bytes.data()->synapse_pos() == synapse_pos);
    REQUIRE(cmd_from_bytes.data()->way_point() == default_pos);
    REQUIRE(cmd_from_bytes.data()->msg() == "");
    REQUIRE(cmd_from_bytes.data()->num() == 1);
  }

  SECTION ("Test create set_wps-command with all data", "[msgpack]") {
    std::shared_ptr<Data> data = std::make_shared<SetWayPoints>(synapse_pos);
    Command cmd(command, data);
    cmd.data()->set_way_point(way_point);
    cmd.data()->set_centered_positions(centered_positions);
    cmd.data()->set_current_way(current_way);
    cmd.data()->set_current_waypoints(current_waypoints);
    cmd.data()->set_msg(msg);
    cmd.data()->set_num(num);
    REQUIRE(cmd.command() == command);
    REQUIRE(cmd.data()->centered_positions() == centered_positions);
    REQUIRE(cmd.data()->current_way() == current_way);
    REQUIRE(cmd.data()->current_waypoints() == current_waypoints);
    REQUIRE(cmd.data()->synapse_pos() == synapse_pos);
    REQUIRE(cmd.data()->way_point() == way_point);
    REQUIRE(cmd.data()->msg() == msg);
    REQUIRE(cmd.data()->num() == num);
    auto payload = cmd.bytes();

    Command cmd_from_bytes(payload.c_str(), payload.size());
    REQUIRE(cmd_from_bytes.command() == command);
    REQUIRE(cmd_from_bytes.data()->centered_positions() == centered_positions);
    REQUIRE(cmd_from_bytes.data()->current_way() == current_way);
    REQUIRE(cmd_from_bytes.data()->current_waypoints() == current_waypoints);
    REQUIRE(cmd_from_bytes.data()->synapse_pos() == synapse_pos);
    REQUIRE(cmd_from_bytes.data()->way_point() == way_point);
    REQUIRE(cmd_from_bytes.data()->msg() == msg);
    REQUIRE(cmd_from_bytes.data()->num() == num);
  }
}

TEST_CASE("Test creating set_target-dto", "[msgpack]") {
  std::string command = "set_target";
  short unit = IPSP;
  position_t default_pos = {-1, -1};
  position_t synapse_pos = {40, 40};
  position_t start_pos= {56, 12};
  position_t target = {56, 12};
  std::vector<position_t> enemy_units = {{40, 40}, {50, 50}};
  std::vector<position_t> target_positions = {{20, 4}, {5, 20}};

  SECTION ("Test create set_target-command with only synapse_pos and unit", "[msgpack]") {
    std::shared_ptr<Data> data = std::make_shared<SetTarget>(synapse_pos, unit);
    Command cmd(command, data);
    REQUIRE(cmd.command() == command);
    REQUIRE(cmd.data()->unit() == unit);
    REQUIRE(cmd.data()->synapse_pos() == synapse_pos);
    REQUIRE(cmd.data()->target() == default_pos);
    REQUIRE(cmd.data()->start_pos() == default_pos);
    REQUIRE(cmd.data()->enemy_units().size() == 0);
    REQUIRE(cmd.data()->target_positions().size() == 0);
    auto payload = cmd.bytes();

    Command cmd_from_bytes(payload.c_str(), payload.size());
    REQUIRE(cmd_from_bytes.command() == command);
    REQUIRE(cmd_from_bytes.data()->unit() == unit);
    REQUIRE(cmd_from_bytes.data()->synapse_pos() == synapse_pos);
    REQUIRE(cmd_from_bytes.data()->target() == default_pos);
    REQUIRE(cmd_from_bytes.data()->start_pos() == default_pos);
    REQUIRE(cmd_from_bytes.data()->enemy_units().size() == 0);
    REQUIRE(cmd_from_bytes.data()->target_positions().size() == 0);
  }

  SECTION ("Test create set_target-command with all data", "[msgpack]") {
    std::shared_ptr<Data> data = std::make_shared<SetTarget>(synapse_pos, unit);
    Command cmd(command, data);
    cmd.data()->set_start_pos(start_pos);
    cmd.data()->set_target(target);
    cmd.data()->set_enemy_units(enemy_units);
    cmd.data()->set_target_positions(target_positions);
    REQUIRE(cmd.command() == command);
    REQUIRE(cmd.data()->unit() == unit);
    REQUIRE(cmd.data()->synapse_pos() == synapse_pos);
    REQUIRE(cmd.data()->target() == target);
    REQUIRE(cmd.data()->start_pos() == start_pos);
    REQUIRE(cmd.data()->enemy_units() == enemy_units);
    REQUIRE(cmd.data()->target_positions() == target_positions);
    auto payload = cmd.bytes();

    Command cmd_from_bytes(payload.c_str(), payload.size());
    REQUIRE(cmd_from_bytes.command() == command);
    REQUIRE(cmd_from_bytes.data()->unit() == unit);
    REQUIRE(cmd_from_bytes.data()->synapse_pos() == synapse_pos);
    REQUIRE(cmd_from_bytes.data()->target() == target);
    REQUIRE(cmd_from_bytes.data()->start_pos() == start_pos);
    REQUIRE(cmd_from_bytes.data()->enemy_units() == enemy_units);
    REQUIRE(cmd_from_bytes.data()->target_positions() == target_positions);
  }
}

TEST_CASE("Test creating toggle_swarm_attack-dto", "[msgpack]") {
  std::string command = "toggle_swarm_attack";
  position_t synapse_pos = {40, 40};

  std::shared_ptr<Data> data = std::make_shared<ToggleSwarmAttack>(synapse_pos);
  Command cmd(command, data);
  REQUIRE(cmd.command() == command);
  REQUIRE(cmd.data()->synapse_pos() == synapse_pos);
  auto payload = cmd.bytes();

  Command cmd_from_bytes(payload.c_str(), payload.size());
  REQUIRE(cmd_from_bytes.command() == command);
  REQUIRE(cmd_from_bytes.data()->synapse_pos() == synapse_pos);
}

TEST_CASE("Test creating get_positions-dto", "[msgpack]") {
  std::string command = "get_positions";
  position_t synapse_pos = {40, 40};
  position_t default_pos = {-1, -1};
  std::vector<position_t> positions = {{40, 40}, {34, 43}};

  SECTION ("Test with single position-request and no (empty) data", "[msgpack]") {
    std::map<short, Data::PositionInfo> position_requests = {{{PLAYER, Data::PositionInfo(SYNAPSE)}}};
    std::string return_cmd = "select_synapse";
    std::shared_ptr<Data> select_synapse = std::make_shared<SelectSynapse>();
    std::shared_ptr<Data> data = std::make_shared<GetPositions>(return_cmd, position_requests, select_synapse);
    Command cmd(command, data);
    REQUIRE(cmd.command() == command);
    REQUIRE(cmd.data()->return_cmd() == return_cmd);
    REQUIRE(cmd.data()->position_requests().size() == position_requests.size());
    for (const auto& it : position_requests) {
      REQUIRE(cmd.data()->position_requests()[it.first]._pos == it.second._pos);
      REQUIRE(cmd.data()->position_requests()[it.first]._unit == it.second._unit);
    }
    REQUIRE(cmd.data()->data()->synapse_pos() == default_pos);
    REQUIRE(cmd.data()->data()->positions() == select_synapse->positions()); // unititialized, but should still work.
    auto payload = cmd.bytes();

    // Read complete get_positions-request.
    Command cmd_from_bytes(payload.c_str(), payload.size());
    REQUIRE(cmd_from_bytes.data()->return_cmd() == return_cmd);
    REQUIRE(cmd_from_bytes.data()->position_requests().size() == position_requests.size());
    for (const auto& it : position_requests) {
      REQUIRE(cmd_from_bytes.data()->position_requests()[it.first]._pos == it.second._pos);
      REQUIRE(cmd_from_bytes.data()->position_requests()[it.first]._unit == it.second._unit);
    }
    REQUIRE(cmd_from_bytes.data()->data()->synapse_pos() == default_pos);
    REQUIRE(cmd_from_bytes.data()->data()->positions() == select_synapse->positions()); 

    // write and then read only get_position-request data
    payload = Command(cmd.data()->return_cmd(), cmd_from_bytes.data()->data()).bytes();
    Command cmd_from_bytes_2(payload.c_str(), payload.size());
    REQUIRE(cmd_from_bytes_2.data()->synapse_pos() == default_pos);
    REQUIRE(cmd_from_bytes_2.data()->positions() == select_synapse->positions()); 
  }

  SECTION ("Test with single position-request", "[msgpack]") {
    std::map<short, Data::PositionInfo> position_requests = {{{PLAYER, Data::PositionInfo(SYNAPSE)}}};
    std::string return_cmd = "select_synapse";
    std::shared_ptr<Data> select_synapse = std::make_shared<SelectSynapse>(positions);
    std::shared_ptr<Data> data = std::make_shared<GetPositions>(return_cmd, position_requests, select_synapse);
    Command cmd(command, data);
    REQUIRE(cmd.command() == command);
    REQUIRE(cmd.data()->return_cmd() == return_cmd);
    REQUIRE(cmd.data()->position_requests().size() == position_requests.size());
    for (const auto& it : position_requests) {
      REQUIRE(cmd.data()->position_requests()[it.first]._pos == it.second._pos);
      REQUIRE(cmd.data()->position_requests()[it.first]._unit == it.second._unit);
    }
    REQUIRE(cmd.data()->data()->synapse_pos() == default_pos);
    REQUIRE(cmd.data()->data()->positions() == select_synapse->positions()); // unititialized, but should still work.
    auto payload = cmd.bytes();

    // Read complete get_positions-request.
    Command cmd_from_bytes(payload.c_str(), payload.size());
    REQUIRE(cmd_from_bytes.data()->return_cmd() == return_cmd);
    REQUIRE(cmd_from_bytes.data()->position_requests().size() == position_requests.size());
    for (const auto& it : position_requests) {
      REQUIRE(cmd_from_bytes.data()->position_requests()[it.first]._pos == it.second._pos);
      REQUIRE(cmd_from_bytes.data()->position_requests()[it.first]._unit == it.second._unit);
    }
    REQUIRE(cmd_from_bytes.data()->data()->synapse_pos() == default_pos);
    REQUIRE(cmd_from_bytes.data()->data()->positions() == select_synapse->positions()); 

    // write and then read only get_position-request data
    payload = Command(cmd.data()->return_cmd(), cmd_from_bytes.data()->data()).bytes();
    Command cmd_from_bytes_2(payload.c_str(), payload.size());
    REQUIRE(cmd_from_bytes_2.data()->synapse_pos() == default_pos);
    REQUIRE(cmd_from_bytes_2.data()->positions() == select_synapse->positions()); 
  }

  SECTION ("Test with multiple position-request", "[msgpack]") {
    std::map<short, Data::PositionInfo> position_requests = {{{CURRENT_WAY, Data::PositionInfo(synapse_pos)}, 
          {CURRENT_WAY_POINTS, Data::PositionInfo(synapse_pos)}, {CENTER, Data::PositionInfo()}}};
    std::string return_cmd = "set_wps";
    std::shared_ptr<Data> set_wps = std::make_shared<SetWayPoints>(synapse_pos);
    std::shared_ptr<Data> data = std::make_shared<GetPositions>(return_cmd, position_requests, set_wps);
    Command cmd(command, data);
    REQUIRE(cmd.command() == command);
    REQUIRE(cmd.data()->return_cmd() == return_cmd);
    REQUIRE(cmd.data()->position_requests().size() == position_requests.size());
    for (const auto& it : position_requests) {
      REQUIRE(cmd.data()->position_requests()[it.first]._pos == it.second._pos);
      REQUIRE(cmd.data()->position_requests()[it.first]._unit == it.second._unit);
    }
    REQUIRE(cmd.data()->data()->synapse_pos() == synapse_pos);
    REQUIRE(cmd.data()->data()->msg() == "");
    REQUIRE(cmd.data()->data()->num() == 1);
    auto payload = cmd.bytes();

    Command cmd_from_bytes(payload.c_str(), payload.size());
    REQUIRE(cmd_from_bytes.data()->return_cmd() == return_cmd);
    REQUIRE(cmd_from_bytes.data()->position_requests().size() == position_requests.size());
    for (const auto& it : position_requests) {
      REQUIRE(cmd_from_bytes.data()->position_requests()[it.first]._pos == it.second._pos);
      REQUIRE(cmd_from_bytes.data()->position_requests()[it.first]._unit == it.second._unit);
    }
    REQUIRE(cmd_from_bytes.data()->data()->synapse_pos() == synapse_pos);
    REQUIRE(cmd.data()->data()->msg() == "");
    REQUIRE(cmd.data()->data()->num() == 1);
  }
}

TEST_CASE("Test creating check-send-audio-dto", "[msgpack]") {
  std::string command = "audio_map";
  std::string map_path = "data/songs/mysong.mp3";

  SECTION ("Test creating check-send-audio-dto on same devive", "[msgpack]") { 
    bool same_device = true;
    std::shared_ptr<Data> data = std::make_shared<CheckSendAudio>(map_path);
    Command cmd(command, data);
    REQUIRE(cmd.command() == command);
    REQUIRE(cmd.data()->same_device() == same_device);
    REQUIRE(cmd.data()->map_path() == map_path);
    REQUIRE(cmd.data()->audio_file_name() == "");
    auto payload = cmd.bytes();

    Command cmd_from_bytes(payload.c_str(), payload.size());
    REQUIRE(cmd_from_bytes.command() == command);
    REQUIRE(cmd_from_bytes.data()->same_device() == same_device);
    REQUIRE(cmd_from_bytes.data()->map_path() == map_path);
    REQUIRE(cmd_from_bytes.data()->audio_file_name() == "");
  }

  SECTION ("Test creating check-send-audio-dto not on same devive", "[msgpack]") { 
    bool same_device = false;
    std::string audio_file_name = "mysong.mp3";
    std::shared_ptr<Data> data = std::make_shared<CheckSendAudio>(map_path, audio_file_name);
    Command cmd(command, data);
    REQUIRE(cmd.command() == command);
    REQUIRE(cmd.data()->same_device() == same_device);
    REQUIRE(cmd.data()->map_path() == map_path);
    REQUIRE(cmd.data()->audio_file_name() == audio_file_name);
    auto payload = cmd.bytes();

    Command cmd_from_bytes(payload.c_str(), payload.size());
    REQUIRE(cmd_from_bytes.command() == command);
    REQUIRE(cmd_from_bytes.data()->same_device() == same_device);
    REQUIRE(cmd_from_bytes.data()->map_path() == map_path);
    REQUIRE(cmd_from_bytes.data()->audio_file_name() == audio_file_name);
  }
}

TEST_CASE("Test creating check-send-audio-data-dto", "[msgpack]") {
  std::string command = "send_audio_data";
  std::string username = "fux";
  std::string songname = "Union v2.mp3";
  int part = 1;
  int parts = 2;
  std::string content = "hello I am audio content";
  SECTION ("Test creating check-send-audio-data-dto", "[msgpack]") {
    std::shared_ptr<Data> data = std::make_shared<AudioTransferDataNew>(username, songname);
    data->set_parts(parts);
    data->set_part(part);
    data->set_content(content);
    Command cmd(command, data);
    REQUIRE(cmd.command() == command);
    REQUIRE(cmd.data()->username() == username);
    REQUIRE(cmd.data()->songname() == songname);
    REQUIRE(cmd.data()->part() == part);
    REQUIRE(cmd.data()->parts() == parts);
    REQUIRE(cmd.data()->content() == content);
    auto payload = cmd.bytes();

    Command cmd_from_bytes(payload.c_str(), payload.size());
    REQUIRE(cmd_from_bytes.command() == command);
    REQUIRE(cmd_from_bytes.data()->username() == username);
    REQUIRE(cmd_from_bytes.data()->songname() == songname);
    REQUIRE(cmd_from_bytes.data()->part() == part);
    REQUIRE(cmd_from_bytes.data()->parts() == parts);
    REQUIRE(cmd_from_bytes.data()->content() == content);
  }
}

TEST_CASE("Test creating initialize-dto", "[msgpack]") {
  std::string command = "initialize_game";
  std::string map_name = "Hear_My_Call-coffeeshoppers";
  std::string source_path = "dissonance/data/analysis/18062755743724368886Hear_My_Call-coffeeshoppers.json";
  nlohmann::json analysed_data = utils::LoadJsonFromDisc(source_path);

  SECTION("Test creating initialize-dto without ais", "[msgpack]") {
    std::shared_ptr<Data> data = std::make_shared<InitializeGame>(map_name);
    Command cmd(command, data);
    REQUIRE(cmd.command() == command);
    REQUIRE(cmd.data()->map_name() == map_name);
    REQUIRE(cmd.data()->ai_audio_data().size() == 0);
    auto payload = cmd.bytes();

    Command cmd_from_bytes(payload.c_str(), payload.size());
    REQUIRE(cmd_from_bytes.command() == command);
    REQUIRE(cmd_from_bytes.data()->map_name() == map_name);
    REQUIRE(cmd_from_bytes.data()->ai_audio_data().size() == 0);
  }
  
  SECTION("Test creating initialize-dto with ais", "[msgpack]") {
    std::shared_ptr<Data> data = std::make_shared<InitializeGame>(map_name);
    data->AddAiAudioData(source_path, analysed_data);
    Command cmd(command, data);
    REQUIRE(cmd.command() == command);
    REQUIRE(cmd.data()->map_name() == map_name);
    REQUIRE(cmd.data()->ai_audio_data()[source_path].dump() == analysed_data.dump());
    auto payload = cmd.bytes();

    Command cmd_from_bytes(payload.c_str(), payload.size());
    REQUIRE(cmd_from_bytes.command() == command);
    REQUIRE(cmd_from_bytes.data()->map_name() == map_name);
    REQUIRE(cmd_from_bytes.data()->ai_audio_data()[source_path].dump() == analysed_data.dump());
  }
}

TEST_CASE("Test creating distributed_iron-dto", "[msgpack]") {
  unsigned int short resource = POTASSIUM;

  SECTION ("Test creating add-iron-cmd", "[msgpack]") {
    std::string command = "add_iron";
    std::shared_ptr<Data> data = std::make_shared<DistributeIron>(resource);
    Command cmd(command, data);
    REQUIRE(cmd.command() == command);
    REQUIRE(cmd.data()->resource() == resource);
    auto payload = cmd.bytes();

    Command cmd_from_bytes(payload.c_str(), payload.size());
    REQUIRE(cmd_from_bytes.command() == command);
    REQUIRE(cmd_from_bytes.data()->resource() == resource);
  }
  
  SECTION ("Test creating remove-iron-cmd", "[msgpack]") {
    std::string command = "remove_iron";
    std::shared_ptr<Data> data = std::make_shared<DistributeIron>(resource);
    Command cmd(command, data);
    REQUIRE(cmd.command() == command);
    REQUIRE(cmd.data()->resource() == resource);
    auto payload = cmd.bytes();

    Command cmd_from_bytes(payload.c_str(), payload.size());
    REQUIRE(cmd_from_bytes.command() == command);
    REQUIRE(cmd_from_bytes.data()->resource() == resource);
  }
}

TEST_CASE("Test creating add_technology-dto", "[msgpack]") {
  unsigned int short technology = SWARM;
  std::string command = "add_technology";

  std::shared_ptr<Data> data = std::make_shared<AddTechnology>(technology);
  Command cmd(command, data);
  REQUIRE(cmd.command() == command);
  REQUIRE(cmd.data()->technology() == technology);
  auto payload = cmd.bytes();

  Command cmd_from_bytes(payload.c_str(), payload.size());
  REQUIRE(cmd_from_bytes.command() == command);
  REQUIRE(cmd_from_bytes.data()->technology() == technology);
}

TEST_CASE("Test creating send_audio_info-dto", "[msgpack]") {
  std::string command = "send_audio_info";
  bool send_audio = false;
  bool send_ai_audios = false;

  std::shared_ptr<Data> data = std::make_shared<SendAudioInfo>(send_audio, send_ai_audios);
  Command cmd(command, data);
  REQUIRE(cmd.command() == command);
  REQUIRE(cmd.data()->send_audio() == send_audio);
  REQUIRE(cmd.data()->send_ai_audios() == send_ai_audios);
  auto payload = cmd.bytes();

  Command cmd_from_bytes(payload.c_str(), payload.size());
  REQUIRE(cmd_from_bytes.command() == command);
  REQUIRE(cmd_from_bytes.data()->send_audio() == send_audio);
}
