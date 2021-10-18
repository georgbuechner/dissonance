#include "game.h"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstddef>
#include<cstdlib>
#include <curses.h>
#include <exception>
#include <filesystem>
#include <map>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include <string>
#include <time.h>
#include <thread>
#include <vector>

#include "constants/codes.h"
#include "nlohmann/json.hpp"
#include "objects/units.h"
#include "player/player.h"
#include "random/random.h"
#include "spdlog/spdlog.h"
#include "utils/utils.h"

#define LINE_HELP 13 

#define RESOURCE_UPDATE_FREQUENCY 500
#define UPDATE_FREQUENCY 50

#define KI_NEW_SOLDIER 1250
#define KI_NEW_SOLDIER 1250

std::string CheckMissingResources(Costs missing_costs) {
  std::string res = "";
  if (missing_costs.size() == 0)
    return res;
  for (const auto& it : missing_costs)
    res += "Missing " + std::to_string(it.second) + " " + resources_name_mapping.at(it.first) + "! ";
  return res;
}

Game::Game(int lines, int cols, int left_border, std::string base_path) 
  : game_over_(false), pause_(false), resigned_(false), audio_(base_path), base_path_(base_path), 
  lines_(lines), cols_(cols), left_border_(left_border) {

  spdlog::get(LOGGER)->info("Loading music paths at {}", base_path + "/settings/music_paths.json");
  std::vector<std::string> paths = utils::LoadJsonFromDisc(base_path + "/settings/music_paths.json");
  spdlog::get(LOGGER)->info("Got music paths: {}", paths.size());

  for (const auto& it : paths) {
    if (it.find("$(HOME)") != std::string::npos)
      audio_paths_.push_back(getenv("HOME") + it.substr(it.find("/")));
    else if (it.find("$(DISSONANCE)") != std::string::npos)
      audio_paths_.push_back(base_path + it.substr(it.find("/")));
    else
      audio_paths_.push_back(it);
  }
}

void Game::play() {
  spdlog::get(LOGGER)->info("Started game with {}, {}, {}, {}", lines_, cols_, LINES, COLS);

  if (LINES < lines_+20 || COLS < (cols_*2)+40) { 
    texts::paragraphs_t paragraphs = {{
        {"Your terminal size is to small to play the game properly."},
        {"Expected width: " + std::to_string(cols_*2+50) + ", actual: " + std::to_string(COLS)},
        {"Expected hight: " + std::to_string(lines_+20) + ", actual: " + std::to_string(LINES)},
    }};
    PrintCentered(paragraphs);
    clear();
    PrintCentered(LINES/2, "Can you increase? (Enter play anyway, q to quit)");
    char c = getch();
    if (c == 'q')
      return;
  }

  // Print welcome text.
  PrintCentered(texts::welcome);

  // Select difficulty
  difficulty_ = 1;

  // select song. 
  std::string source_path = SelectAudio();
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

    field_ = new Field(lines_, cols_, ran_gen, left_border_);
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
    PrintCentered({{"Game cannot be played with this song, as map is unplayable. "
        "It might work with a higher resolution. (dissonance -r)"}});
    return;
  }
  PrintCentered({{"Lowered denceness of map by " + std::to_string(denceness) + " to make map playable"}});

  // Setup players.
  player_one_ = new Player(nucleus_pos_1, field_, ran_gen, resource_positions_1);
  player_two_ = new AudioKi(nucleus_pos_2, field_, &audio_, ran_gen, resource_positions_2);
  player_one_->set_enemy(player_two_);
  player_two_->set_enemy(player_one_);
  player_two_->SetUpTactics(true); 

  // Let player one distribute initial iron.
  DistributeIron();
  // Let player two distribute initial iron.
  player_two_->DistributeIron(Resources::OXYGEN);
  player_two_->DistributeIron(Resources::OXYGEN);
  player_two_->HandleIron(audio_.analysed_data().data_per_beat_.front());

  // Start game
  audio_.play();
  std::thread thread_actions([this]() { RenderField(); });
  std::thread thread_choices([this]() { (GetPlayerChoice()); });
  std::thread thread_ki([this]() { (HandleActions()); });
  thread_actions.join();
  thread_choices.join();
  thread_ki.join();
}

void Game::RenderField() {
  spdlog::get(LOGGER)->debug("Game::RenderField: started");
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
 
  while (!game_over_) {
    auto cur_time = std::chrono::steady_clock::now();

    if (pause_) continue;
    
    // Analyze audio data.
    auto elapsed = utils::GetElapsed(audio_start_time, cur_time);
    auto data_at_beat = data_per_beat.front();
    if (elapsed >= data_at_beat.time_) {
      render_frequency = 60000.0/(data_at_beat.bpm_*16);
      ki_resource_update_frequency = (60000.0/data_at_beat.bpm_); //*(data_at_beat.level_/50.0);
      player_resource_update_freqeuncy = 60000.0/(static_cast<double>(data_at_beat.bpm_)/2);
    
      off_notes = audio_.MoreOffNotes(data_at_beat);
      data_per_beat.pop_front();
      played_levels_.push_back(audio_.analysed_data().average_level_-data_at_beat.level_);
    }

    if (player_two_->HasLost() || player_one_->HasLost() || data_per_beat.size() == 0) {
      SetGameOver((player_two_->HasLost()) ? "YOU WON" : "YOU LOST");
      audio_.Stop();
      break;
    }
    if (resigned_) {
      SetGameOver("YOU RESIGNED");
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
      PrintFieldAndStatus();
      last_update = cur_time;
    }
  } 
}

void Game::HandleActions() {
  spdlog::get(LOGGER)->debug("Game::HandleActions: started");
  auto audio_start_time = std::chrono::steady_clock::now();
  auto analysed_data = audio_.analysed_data();
  std::list<AudioDataTimePoint> data_per_beat = analysed_data.data_per_beat_;

  // Handle building neurons and potentials.
  while(!game_over_) {
    auto cur_time = std::chrono::steady_clock::now(); 

    if (pause_) continue;

    // Analyze audio data.
    auto elapsed = utils::GetElapsed(audio_start_time, cur_time);
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

void Game::GetPlayerChoice() {
  spdlog::get(LOGGER)->debug("Game::GetPlayerChoice: started");
  int choice;
  PrintFieldAndStatus();
  while (true) {

    choice = getch();
    PrintMessage("", false);  // clear message line after each input.
    // q: quit game
    if (choice == 'q') {
      pause_ = true;
      ClearField();
      PrintCentered(LINES/2, "Are you sure you want to exist? y/n");
      char c = getch();
      pause_ = false;
      if (c == 'y') {
        resigned_ = true;
        break;
      }
    }

    // Stop any other player-inputs, if game is over.
    else if (game_over_) {
      continue;
    }

    // SPACE: pause/ unpause game
    else if (choice == ' ') {
      if (pause_) {
        audio_.Unpause();
        PrintMessage("Un-Paused game.", false);
      }
      else {
        audio_.Pause();
        PrintMessage("Paused game.", false);
      }
      pause_ = !pause_;
    }

    else if (pause_) {
      continue;
    }

    else if (choice == 'h') {
      pause_ = true;
      PrintCentered(texts::help);
      pause_ = false;
    }

    else if (choice == 'c') {
      for (int i=0; i<10; i++)
        player_one_->IncreaseResources(true);
    }

    // e: add epsp
    else if (choice == 'e') {
      std::string res = CheckMissingResources(player_one_->GetMissingResources(UnitsTech::EPSP));
      if (res != "")
        PrintMessage(res, true);
      else {
        auto pos = SelectNeuron(player_one_, UnitsTech::SYNAPSE);
        if (pos.first == -1)
          PrintMessage("Invalid choice!", true);
        else {
          PrintMessage("Added epsp @synapse " + utils::PositionToString(pos), false);
          player_one_->AddPotential(pos, UnitsTech::EPSP);
        }
      }
    }
    // i: add ipsp 
    else if (choice == 'i') {
      std::string res = CheckMissingResources(player_one_->GetMissingResources(UnitsTech::IPSP));
      if (res != "")
        PrintMessage(res, true);
      else {
        auto pos = SelectNeuron(player_one_, UnitsTech::SYNAPSE);
        if (pos.first == -1)
          PrintMessage("Invalid choice!", true);
        else {
          PrintMessage("created ipsp @synapse: " + utils::PositionToString(pos), false);
          spdlog::get(LOGGER)->debug("Game::GetPlayerChoice: Creating potential.");
          player_one_->AddPotential(pos, UnitsTech::IPSP);
        }
      }
    }

    // S: Synapse
    else if (choice == 'S') {
      std::string res = CheckMissingResources(player_one_->GetMissingResources(UnitsTech::SYNAPSE));
      if (res != "") 
        PrintMessage(res, true);
      else {
        position_t nucleus_pos = SelectNucleus(player_one_);
        if (nucleus_pos.first == -1)
          PrintMessage("Invalid choice!", true);
        else {
          PrintMessage("User the arrow keys to select a position. Press Enter to select.", false);
          position_t pos = SelectPosition(nucleus_pos, player_one_->cur_range());
          if (pos.first != -1) {
            position_t epsp_target = player_two_->GetOneNucleus();
            position_t ipsp_target = player_two_->GetRandomNeuron(); // random tower.
            player_one_->AddNeuron(pos, UnitsTech::SYNAPSE, epsp_target, ipsp_target);
            field_->AddNewUnitToPos(pos, UnitsTech::SYNAPSE);
          }
          PrintMessage(res, res!="");
        }
      }
    }

    // D: place defence-tower
    else if (choice == 'A') {
      std::string res = CheckMissingResources(player_one_->GetMissingResources(UnitsTech::ACTIVATEDNEURON));
      if (res != "") 
        PrintMessage(res, true);
      else {
        position_t nucleus_pos = SelectNucleus(player_one_);
        if (nucleus_pos.first == -1)
          PrintMessage("Invalid choice!", true);
        else {
          PrintMessage("User the arrow keys to select a position. Press Enter to select.", false);
          position_t pos = SelectPosition(nucleus_pos, player_one_->cur_range());
          if (pos.first != -1) {
            player_one_->AddNeuron(pos, UnitsTech::ACTIVATEDNEURON);
            field_->AddNewUnitToPos(pos, UnitsTech::ACTIVATEDNEURON);
          }
          PrintMessage(res, res!="");
        }
      }
    }

    // N: new nucleus
    else if (choice == 'N') {
      spdlog::get(LOGGER)->debug("Game::AddNucleus");
      auto num_nucleus = player_one_->GetAllPositionsOfNeurons(UnitsTech::NUCLEUS).size();
      spdlog::get(LOGGER)->debug("Game::AddNucleus: current num of nucleus: {}", num_nucleus);
      std::string res = CheckMissingResources(
          player_one_->GetMissingResources(UnitsTech::NUCLEUS, num_nucleus));
      spdlog::get(LOGGER)->debug("Game::AddNucleus: missing resources: {}", res);
      if (res != "") 
        PrintMessage(res, true);
      else {
        PrintMessage("User the arrow keys to select a position. Press Enter to select.", false);
        auto start_position = SelectFieldPositionByAlpha(field_->GetAllCenterPositionsOfSections(), "Select start");
        if (start_position.first != -1 && start_position.second != -1) {
          position_t pos = SelectPosition(start_position, ViewRange::GRAPH);
          if (pos.first != -1) {
            player_one_->AddNeuron(pos, UnitsTech::NUCLEUS);
            field_->AddNewUnitToPos(pos, UnitsTech::NUCLEUS);
          }
        }
        PrintMessage(res, res!="");
      }
    }

    // T: Technology
    else if (choice == 't') {
      pause_ = true;
      choice_mapping_t mapping;
      for (const auto& it : player_one_->technologies()) {
        size_t color = COLOR_DEFAULT;
        size_t missing_costs = player_one_->GetMissingResources(it.first, it.second.first+1).size();
        if (it.second.first < it.second.second && missing_costs == 0)
          color = COLOR_AVAILIBLE;
        mapping[it.first-UnitsTech::WAY] = {units_tech_mapping.at(it.first) 
          + " (" + utils::PositionToString(it.second) + ")", color};
      }
      int technology = SelectInteger("Select technology", true, mapping, {3, 5, 8, 10, 11})
        +UnitsTech::WAY;
      if (player_one_->AddTechnology(technology))
        PrintMessage("selected: " + units_tech_mapping.at(technology), false);
      else if (technology != -1)
        PrintMessage("Not enough resources or inavlid selection", true);
      pause_ = false;
    }

    else if (choice == 's') {
      auto all_synapse_position = player_one_->GetAllPositionsOfNeurons(UnitsTech::SYNAPSE);
      if (all_synapse_position.size() == 0) {
        PrintMessage("You do not have any synapses yet. Use [S] to create one!", true);
        continue;
      }
      position_t pos = all_synapse_position.front(); 
      if (all_synapse_position.size() > 1) 
        pos = SelectNeuron(player_one_, UnitsTech::SYNAPSE);
      if (pos.first == -1)
        PrintMessage("Invalid choice!", true);
      else {
        // Create options and get player-choice.
        auto mapping = player_one_->GetOptionsForSynapes(pos);
        int choice = SelectInteger("What to do?", true, mapping, {mapping.size()});

        // Do action after getting choice.
        if (mapping.count(choice) > 0) {
          // if func=swarm, simply turn on/ off.
          if (choice == 4)
            player_one_->SwitchSwarmAttack(pos);
          // Otherwise: way-selection (full map)
          else if (choice == 0 || choice == 1) {
            auto start_position = SelectFieldPositionByAlpha(
                field_->GetAllCenterPositionsOfSections(), "Select start");
            if (start_position.first != -1) {
              auto new_pos = SelectPosition(start_position, ViewRange::GRAPH);
              if (new_pos.first != -1) {
                if (choice == 0)
                  player_one_->ResetWayForSynapse(pos, new_pos);
                else if (choice == 1)
                  player_one_->AddWayPosForSynapse(pos, new_pos);
              }
            }
          }
          // Otherwise: target-selection (enemy-nucleus)
          else {
            auto new_pos = SelectPosition(player_two_->GetOneNucleus(), ViewRange::GRAPH);
            if (new_pos.first != -1 && choice== 2)
              player_one_->ChangeIpspTargetForSynapse(pos, new_pos);
            else if (new_pos.first != -1 && choice == 3)
              player_one_->ChangeEpspTargetForSynapse(pos, new_pos);
          }
        }
        else
          PrintMessage("Invalid choice: " + std::to_string(choice), true);
      }
    }

    else if (choice == 'd') {
      DistributeIron();
    }
  } 
}

position_t Game::SelectPosition(position_t start, int range) {
  spdlog::get(LOGGER)->info("Game::SelectPosition");
  bool end = false;
  position_t new_pos = {-1, -1};
  // Make sure than position exists.
  if (start.first == -1 && start.second == -1)
    return new_pos;
  field_->set_highlight({start});
  field_->set_range(range);
  field_->set_range_center(start);
  
  while(!game_over_ && !end) {
    int choice = getch();
    new_pos = field_->highlight().front();

    if (std::to_string(choice) == "10") {
      if (field_->GetSymbolAtPos(new_pos) == SYMBOL_FREE || range == ViewRange::GRAPH) 
        end = true;
      else 
        PrintMessage("Invalid position (not free)!", false); 
    }
    else if (choice == 'q') {
      end = true;
      new_pos = {-1, -1};
    }
    else if (utils::IsUp(choice)) {
      new_pos.first--; 
      PrintFieldAndStatus();
    }
    else if (utils::IsDown(choice)) {
      new_pos.first++; 
      PrintFieldAndStatus();
    }
    else if (utils::IsLeft(choice)) {
      new_pos.second--;
      PrintFieldAndStatus();
    }
    else if (utils::IsRight(choice)) {
      new_pos.second++;
      PrintFieldAndStatus();
    }
    
    // Update highlight only if in range.
    if (field_->InRange(new_pos, range, start) || range == ViewRange::GRAPH) {
      field_->set_highlight({new_pos});
      PrintFieldAndStatus();
    }
    else 
      PrintMessage("Outside of range!", false); 
  }
  field_->set_range(ViewRange::HIDE);
  field_->set_range_center({-1, -1});
  field_->set_highlight({});
  return new_pos;
}

position_t Game::SelectNeuron(Player* p, int type) {
  std::string msg = "Choose " + units_tech_mapping.at(type) + ": ";
  return SelectFieldPositionByAlpha(p->GetAllPositionsOfNeurons(type), msg);
}

position_t Game::SelectFieldPositionByAlpha(std::vector<position_t> positions, std::string msg) {
  std::map<position_t, char> replacements;
  int counter = 0;
  for (auto it : positions)
    replacements[it] = (int)'a'+(counter++);
  field_->set_replace(replacements);
  field_->set_highlight(positions);

  PrintFieldAndStatus();
  PrintMessage(msg.c_str(), false);
  char choice = getch();
  field_->set_replace({});
  field_->set_highlight({});
  
  // Find selected barack in replacements and return.
  for (auto it : replacements) {
    if (it.second == choice)
      return it.first;
  }
  return {-1, -1};
}

void Game::DistributeIron() {
  spdlog::get(LOGGER)->info("Game::DistributeIron.");
  pause_ = true;
  ClearField();
  bool end = false;
  // Get iron and print options.
  PrintCentered(LINES/2-1, "(use 'q' to quit, +/- to add/remove iron and h/l or ←/→ to circle through resources)");
  std::vector<std::string> symbols = {SYMBOL_OXYGEN, SYMBOL_POTASSIUM, SYMBOL_SEROTONIN, SYMBOL_GLUTAMATE, 
    SYMBOL_DOPAMINE, SYMBOL_CHLORIDE};
  position_t c = {LINES/2, COLS/2};
  std::vector<position_t> positions = { {c.first-10, c.second}, {c.first-6, c.second+15}, {c.first+6, c.second+15}, 
    {c.first+10, c.second}, {c.first+6, c.second-15}, {c.first-6, c.second-15}, };

  std::string help = "";
  std::string info = "";
  std::string error = "";
  std::string success = "";

  unsigned int current = 0;
  while(!end) {
    // Get current resource and symbol.
    std::string current_symbol = symbols[current];
    int resource = resources_symbol_mapping.at(current_symbol);

    // Print texts (help, current resource info)
    help = "Iron (FE): " + player_one_->resources().at(IRON).Print() + "";;
    PrintCentered(LINES/2-2, help);
    info = resources_name_mapping.at(resource) + ": FE" 
      + std::to_string(player_one_->resources().at(resource).distributed_iron())
      + ((player_one_->resources().at(resource).Active()) ? " (active)" : " (inactive)");
    PrintCentered(LINES/2+1, info);

    if (error != "") {
      attron(COLOR_ERROR);
      PrintCentered(LINES/2+2, error);
    }
    else if (success != "") {
      attron(COLOR_SUCCESS);
      PrintCentered(LINES/2+2, success);
    }
    attron(COLOR_DEFAULT);
    success = "";
    error = "";

    // Print resource circle.
    for (unsigned int i=0; i<symbols.size(); i++) {
      if (player_one_->resources().at(resources_symbol_mapping.at(symbols[i])).Active())
        attron(COLOR_PAIR(COLOR_SUCCESS));
      if (i == current)
        attron(COLOR_PAIR(COLOR_MARKED));
      mvaddstr(positions[i].first, positions[i].second, symbols[i].c_str());
      attron(COLOR_PAIR(COLOR_DEFAULT));
      refresh();
    }

    // Get players choice.
    char choice = getch();

    if (utils::IsDown(choice))
      current = utils::Mod(current+1, symbols.size());
    else if (utils::IsUp(choice))
      current = utils::Mod(current-1, symbols.size());
    else if (choice == '+' || choice == '-') {
      int resource = (resources_symbol_mapping.count(current_symbol) > 0) 
        ? resources_symbol_mapping.at(current_symbol) : Resources::OXYGEN;
      bool res = (choice == '+') ? player_one_->DistributeIron(resource) 
        : player_one_->RemoveIron(resource);
      if (res)
        success = "Selected!";
      else
        error = "Not enough iron.";
    }
    else if (choice == 'q')
      end = true;

  }
  ClearField();
  pause_ = false;
}

int Game::SelectInteger(std::string msg, bool omit, choice_mapping_t& mapping, std::vector<size_t> splits) {
  spdlog::get(LOGGER)->debug("Game::SelectInteger: {}, size: {}", msg, mapping.size());
  pause_ = true;
  ClearField();
  bool end = false;

  std::vector<std::pair<std::string, int>> options;
  for (const auto& option : mapping) {
    char c_option = 'a'+option.first;
    std::string txt = "";
    txt += c_option; 
    txt += ": " + option.second.first + "    ";
    options.push_back({txt, option.second.second});
  }
  spdlog::get(LOGGER)->debug("Game::SelectInteger: created options {}", options.size());
  
  // Print matching the splits.
  spdlog::get(LOGGER)->debug("Game::SelectInteger: printing in splits {}", splits.size());
  int counter = 0;
  int last_split = 0;
  for (const auto& split : splits) {
    spdlog::get(LOGGER)->debug("Game::SelectInteger: printing upto split {}", split);
    std::vector<std::pair<std::string, int>> option_part; 
    for (unsigned int i=last_split; i<split && i<options.size(); i++)
      option_part.push_back(options[i]);
    spdlog::get(LOGGER)->debug("Game::SelectInteger: printing {} parts", option_part.size());
    PrintCenteredColored(LINES/2+(counter+=2), option_part);
    last_split = split;
  }
  PrintCentered(LINES/2-1, msg);
  PrintCentered(LINES/2+counter+3, "> enter number...");

  while (!end) {
    // Get choice.
    char choice = getch();
    int int_choice = choice-'a';
    if (choice == 'q' && omit)
      end = true;
    else if (mapping.count(int_choice) > 0 && (mapping.at(int_choice).second == COLOR_AVAILIBLE 
          || !omit)) {
      pause_ = false;
      spdlog::get(LOGGER)->debug("Game::SelectInteger: done, retuning: {}", int_choice);
      return int_choice;
    }
    else if (mapping.count(int_choice) > 0 && mapping.at(int_choice).second != COLOR_AVAILIBLE 
        && omit)
      PrintCentered(LINES/2+counter+5, "Selection not available (not enough resources?): " 
          + std::to_string(int_choice));
    else 
      PrintCentered(LINES/2+counter+5, "Wrong selection: " + std::to_string(int_choice));
  }
  pause_ = false;
  return -1;
}

void Game::PrintMessage(std::string msg, bool error) {
  if (error)
    attron(COLOR_PAIR(COLOR_ERROR));
  else
    attron(COLOR_PAIR(COLOR_MSG));
  std::unique_lock ul(mutex_print_field_);
  std::string clear_string(COLS, ' ');
  mvaddstr(lines_+16, 0, clear_string.c_str());
  mvaddstr(lines_+16, left_border_, msg.c_str());
  attron(COLOR_PAIR(COLOR_DEFAULT));
  refresh();
}

void Game::PrintFieldAndStatus() {
  std::unique_lock ul(mutex_print_field_);
  // mvaddstr(LINE_HELP, 10, HELP);
  PrintHelpLine();
  field_->PrintField(player_one_, player_two_);
  
  auto lines = player_one_->GetCurrentStatusLine();
  for (unsigned int i=0; i<lines.size(); i++) {
    mvaddstr(15+i, left_border_ + cols_*2 + 1, lines[i].c_str());
  }

  PrintCentered(1, "DISSONANCE");
  std::string msg = "Enemy potential: (" + player_two_->GetNucleusLive() + ")";
  PrintCentered(2, msg.c_str());

  // Clear music bar.
  std::string clear_string(COLS, ' ');
  for (int i=4; i<13; i++)
    mvaddstr(i, 0, clear_string.c_str());
  // Print music bar.
  auto played_levels = played_levels_;
  int played_levels_len = played_levels.size();
  if (played_levels_len > cols_)
    played_levels = utils::SliceVector(played_levels, played_levels_len-cols_, cols_);
  double percent_played = static_cast<double>(played_levels_len*100)/audio_.analysed_data().data_per_beat_.size();
  if (percent_played < 50)
    attron(COLOR_PAIR(COLOR_MSG));
  else if (percent_played < 80)
    attron(COLOR_PAIR(COLOR_AVAILIBLE));
  else
    attron(COLOR_PAIR(COLOR_ERROR));
  for (unsigned int i=0; i<played_levels.size(); i++) {
    int level = (played_levels[i]*4)/audio_.analysed_data().max_peak_;
    if (level > 4) level = 4;
    if (level < -4) level = -4;
    mvaddstr(8+level, left_border_+cols_/2+i, "-");
  }
  attron(COLOR_DEFAULT);
  
  refresh();
}

void Game::SetGameOver(std::string msg) {
  std::unique_lock ul(mutex_print_field_);
  clear();
  PrintCentered(LINES/2, msg);
  refresh();
  game_over_ = true;
}

void Game::PrintCentered(texts::paragraphs_t paragraphs) {
  std::unique_lock ul(mutex_print_field_);
  for (const auto& paragraph : paragraphs) {
    refresh();
    clear();
    int size = paragraph.size()/2;
    int counter = 0;
    for (const auto& line : paragraph) {
      PrintCentered(LINES/2-size+(counter++), line);
    }
    PrintCentered(LINES/2+size+2, "[Press any key to continue]");
    char c = getch();
    c++;
  }
}

void Game::PrintCentered(int line, std::string txt) {
  std::string clear_string(COLS, ' ');
  mvaddstr(line, 0, clear_string.c_str());
  mvaddstr(line, COLS/2-txt.length()/2, txt.c_str());
}

void Game::PrintCenteredColored(int line, std::vector<std::pair<std::string, int>> txt_with_color) {
  // Get total length.
  unsigned int total_length = 0;
  for (const auto& it : txt_with_color) 
    total_length += it.first.length();

  // Print parts one by one and update color for each part.
  unsigned int position = COLS/2-total_length/2;
  for (const auto& it : txt_with_color) {
    attron(COLOR_PAIR(it.second));
    mvaddstr(line, position, it.first.c_str());
    position += it.first.length();
    attron(COLOR_PAIR(COLOR_DEFAULT));
  }
}

void Game::PrintHelpLine() {

  bool technology_availible = false;
  for (const auto& it : player_one_->technologies()) {
    if (player_one_->GetMissingResources(it.first).size() == 0 && it.second.first < it.second.second) {
      technology_availible = true;
      break;
    }
  }

  std::vector<std::pair<std::string, bool>> parts;
  parts.push_back({"[e]psp", player_one_->GetMissingResources(UnitsTech::EPSP).size() == 0 
      && player_one_->GetAllPositionsOfNeurons(UnitsTech::SYNAPSE).size() > 0});
  parts.push_back({", ", false});
  parts.push_back({"[i]psp", player_one_->GetMissingResources(UnitsTech::IPSP).size() == 0 
      && player_one_->GetAllPositionsOfNeurons(UnitsTech::SYNAPSE).size() > 0});
  parts.push_back({" | ", false});
  parts.push_back({"[A]ctivated neuron", player_one_->GetMissingResources(UnitsTech::ACTIVATEDNEURON).size() == 0});
  parts.push_back({", ", false});
  parts.push_back({"[S]ynapse", player_one_->GetMissingResources(UnitsTech::SYNAPSE).size() == 0});
  parts.push_back({" | ", false});
  parts.push_back({"[d]istribute iron", player_one_->resources().at(Resources::IRON).cur() > 0});
  parts.push_back({", ", false});
  parts.push_back({"[s]elect synapse", player_one_->GetAllPositionsOfNeurons(UnitsTech::SYNAPSE).size() > 0});
  parts.push_back({", ", false});
  parts.push_back({"[t]echnology", technology_availible});
  parts.push_back({" | [h]elp, pause [space], [q]uit", false});

  std::vector<std::pair<std::string, int>> parts_with_color;
  for (const auto& it : parts)
    parts_with_color.push_back({it.first, (it.second) ? COLOR_AVAILIBLE : COLOR_DEFAULT});
  PrintCenteredColored(LINE_HELP, parts_with_color);
}

void Game::ClearField() {
  std::unique_lock ul(mutex_print_field_);
  clear();
  refresh();
}

std::string Game::SelectAudio() {
  ClearField();
  AudioSelector selector = SetupAudioSelector("", "select audio", audio_paths_);
  selector.options_.push_back({"dissonance_recently_played", "recently played"});
  std::vector<std::string> recently_played = utils::LoadJsonFromDisc(base_path_ + "/settings/recently_played.json");
  std::string error = "";
  std::string help = "(use + to add paths, ENTER to select,  h/l or ←/→ to change directory and j/k or ↓/↑ to circle through songs,)";
  unsigned int selected = 0;
  int level = 0;
  unsigned int print_start = 0;
  unsigned int max = LINES/2;
  std::vector<std::pair<std::string, std::string>> visible_options;

  while(true) {
    unsigned int print_max = std::min((unsigned int)selector.options_.size(), max);
    visible_options = utils::SliceVector(selector.options_, print_start, print_max);
    
    PrintCentered(10, utils::ToUpper(selector.title_));
    PrintCentered(11, selector.path_);
    PrintCentered(12, help);

    attron(COLOR_PAIR(COLOR_ERROR));
    PrintCentered(13, error);
    error = "";
    attron(COLOR_PAIR(COLOR_DEFAULT));

    for (unsigned int i=0; i<visible_options.size(); i++) {
      if (i == selected)
        attron(COLOR_PAIR(COLOR_MARKED));
      PrintCentered(15 + i, visible_options[i].second);
      attron(COLOR_PAIR(COLOR_DEFAULT));
    }

    // Get players choice.
    char choice = getch();
    if (utils::IsRight(choice)) {
      level++;
      if (visible_options[selected].first == "dissonance_recently_played")
        selector = SetupAudioSelector("", "Recently Played", recently_played);
      else if (std::filesystem::is_directory(visible_options[selected].first)) {
        selector = SetupAudioSelector(visible_options[selected].first, visible_options[selected].second, 
            utils::GetAllPathsInDirectory(visible_options[selected].first));
        selected = 0;
        print_start = 0;
      }
      else 
        error = "Not a directory!";
    }
    else if (utils::IsLeft(choice)) {
      if (level == 0)
        error = "No parent directory.";
      level--;
      selected = 0;
      print_start = 0;
      if (level == 0) {
        selector = SetupAudioSelector("", "select audio", audio_paths_);
        selector.options_.push_back({"dissonance_recently_played", "recently played"});
      }
      else {
        std::filesystem::path p = selector.path_;
        selector = SetupAudioSelector(p.parent_path().string(), p.parent_path().filename().string(), 
            utils::GetAllPathsInDirectory(p.parent_path()));
      }
    }
    else if (utils::IsDown(choice)) {
      if (selected == print_max-1 && selector.options_.size() > max)
        print_start++;
      else 
        selected = utils::Mod(selected+1, visible_options.size());
    }
    else if (utils::IsUp(choice)) {
      if (selected == 0 && print_start > 0)
        print_start--;
      else 
        selected = utils::Mod(selected-1, print_max);
    }
    else if (std::to_string(choice) == "10") {
      std::filesystem::path select_path = visible_options[selected].first;
      if (select_path.filename().extension() == ".mp3" || select_path.filename().extension() == ".wav")
        break;
      else 
        error = "Wrong file type. Select mp3 or wav";
    }
    else if (choice == '+') {
      std::string input = InputString("Absolute path: ");
      if (std::filesystem::exists(input)) {
        if (input.back() == '/')
          input.pop_back();
        audio_paths_.push_back(input);
        nlohmann::json audio_paths = utils::LoadJsonFromDisc(base_path_ + "/settings/music_paths.json");
        audio_paths.push_back(input);
        utils::WriteJsonFromDisc(base_path_ + "/settings/music_paths.json", audio_paths);
        selector = SetupAudioSelector("", "select audio", audio_paths_);
      }
      else {
        error = "Path does not exist.";
      }
    }
    ClearField();
  }

  // Add selected aubio-file to recently-played files.
  std::filesystem::path select_path = selector.options_[selected].first;
  bool exists=false;
  for (const auto& it : recently_played) {
    if (it == select_path.string()) {
      exists = true;
      break;
    }
  }
  if (!exists)
    recently_played.push_back(select_path.string());
  if (recently_played.size() > 10) 
    recently_played.erase(recently_played.begin());
  nlohmann::json j_recently_played = recently_played;
  utils::WriteJsonFromDisc(base_path_ + "/settings/recently_played.json", j_recently_played);

  // Return selected path.
  return select_path.string(); 
}

Game::AudioSelector Game::SetupAudioSelector(std::string path, std::string title, std::vector<std::string> paths) {
  std::vector<std::pair<std::string, std::string>> options;
  for (const auto& it : paths) {
    std::filesystem::path path = it;
    options.push_back({it, path.filename()});
  }
  return AudioSelector({path, title, options});
}

std::string Game::InputString(std::string msg) {
  ClearField();
  PrintCentered(LINES/2, msg.c_str());
  echo();
  curs_set(1);
  std::string input;
  int ch = getch();
  while (ch != '\n') {
    input.push_back(ch);
    ch = getch();
  }
  noecho();
  curs_set(0);
  return input;
}

position_t Game::SelectNucleus(Player*) {
  auto all_neuron_positions = player_one_->GetAllPositionsOfNeurons(UnitsTech::NUCLEUS);
  position_t nucleus_pos = {-1, -1};
  if (all_neuron_positions.size() > 1)
    nucleus_pos = SelectNeuron(player_one_, UnitsTech::NUCLEUS);
  else if (all_neuron_positions.size() == 1)
    nucleus_pos = all_neuron_positions.front();
  return nucleus_pos;
}
