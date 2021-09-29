#include "game.h"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <curses.h>
#include <exception>
#include <filesystem>
#include <map>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <time.h>
#include <thread>
#include <vector>

#include "constants/codes.h"
#include "objects/units.h"
#include "player/player.h"
#include "spdlog/spdlog.h"
#include "utils/utils.h"

#define HELP "[e]psp, [i]psp | [A]ctivate neuron, build [S]ynapse | [d]istribute iron, [s]elect synapse | [h]elp | pause [space], [q]uit"

#define LINE_HELP 8
#define LINE_STATUS LINES-9
#define LINE_MSG LINES-9

#define RESOURCE_UPDATE_FREQUENCY 500
#define UPDATE_FREQUENCY 50

#define KI_NEW_SOLDIER 1250
#define KI_NEW_SOLDIER 1250

std::string CheckMissingResources(Costs missing_costs) {
  std::string res = "";
  if (missing_costs.size() == 0)
    return res;
  for (const auto& it : missing_costs) {
    res += "Missing " + std::to_string(it.second) + " " + resources_name_mapping.at(it.first) + "! ";
  }
  return res;
}

Game::Game(int lines, int cols, int left_border, std::string audio_base_path) 
  : game_over_(false), pause_(false), audio_base_path_(audio_base_path + "/audio_files/"), 
  lines_(lines), cols_(cols), left_border_(left_border) {}

void Game::play() {
  spdlog::get(LOGGER)->info("Started game with {}, {}, {}, {}", lines_, cols_, LINES, COLS);

  if (LINES < lines_+20 || COLS < (cols_*2)+40) { //|| lines_ < 25 || cols_ < 40) {
    Paragraphs paragraphs = {{
        {"You terminal size is to small to play the game properly."},
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
  PrintCentered(utils::LoadWelcome());

  // Select difficulty
  difficulty_ = 1;

  // select song. 
  choice_mapping_t mappings;
  size_t counter = 0;
  for (const auto& it : std::filesystem::directory_iterator(audio_base_path_))
    mappings[counter++] = {it.path().filename(), COLOR_DEFAULT};
  size_t source_path_index = SelectInteger("Select audio", false, mappings, {3, 7, 11});
  std::string source_path = audio_base_path_ + mappings[source_path_index].first;
  audio_.set_source_path(source_path);
  audio_.Analyze();

  // Build fields
  field_ = new Field(lines_, cols_, left_border_, &audio_);
  field_->AddHills();
  int player_one_section = (int)audio_.analysed_data().average_bpm_%8+1;
  int player_two_section = (int)audio_.analysed_data().average_level_%8+1;
  if (player_one_section == player_two_section)
    player_two_section = (player_two_section+1)%8;
  Position nucleus_pos = field_->AddNucleus(player_one_section);
  nucleus_pos = field_->AddNucleus(player_two_section);

  player_one_ = new Player(nucleus_pos, field_, 3, &audio_);
  player_two_ = new AudioKi(nucleus_pos, 4, field_, &audio_);
  player_one_->set_enemy(player_two_);
  player_two_->set_enemy(player_one_);
  field_->BuildGraph(player_one_->nucleus_pos(), player_two_->nucleus_pos());
  player_two_->SetUpTactics();

  // Let player one distribute initial iron.
  DistributeIron();
  // Let player two distribute initial iron.
  player_two_->DistributeIron(Resources::OXYGEN);
  player_two_->HandleIron(audio_.analysed_data().data_per_beat_.front());

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
    auto elapsed = utils::get_elapsed(audio_start_time, cur_time);
    auto data_at_beat = data_per_beat.front();
    if (elapsed >= data_at_beat.time_) {
      render_frequency = 60000.0/(data_at_beat.bpm_*16);
      ki_resource_update_frequency = (60000.0/data_at_beat.bpm_); //*(data_at_beat.level_/50.0);
      player_resource_update_freqeuncy = 60000.0/data_at_beat.bpm_;
    
      off_notes = audio_.MoreOfNotes(data_at_beat);
      data_per_beat.pop_front();
    }

    if (player_two_->HasLost() || player_one_->HasLost() || data_per_beat.size() == 0) {
      SetGameOver((player_two_->HasLost()) ? "YOU WON" : "YOU LOST");
      audio_.Stop();
      break;
    }
   
    // Increase resources.
    if (utils::get_elapsed(last_resource_player_one, cur_time) > player_resource_update_freqeuncy) {
      player_one_->IncreaseResources(off_notes);
      last_resource_player_one = cur_time;
    }
    if (utils::get_elapsed(last_resource_player_two, cur_time) > ki_resource_update_frequency) {
      player_two_->IncreaseResources(off_notes);
      last_resource_player_two = cur_time;
    }

    if (utils::get_elapsed(last_update, cur_time) > render_frequency) {
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
    auto elapsed = utils::get_elapsed(audio_start_time, cur_time);
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
    // q: quit game
    if (choice == 'q') {
      refresh();
      endwin();
      exit(0);
    }

    // Stop any other player-inputs, if game is over.
    else if (game_over_) {
      continue;
    }

    else if (std::isdigit(choice)) {
      field_->set_highlight(field_->GetAllPositionsOfSection(choice-'0'));
    }

    // SPACE: pause/ unpause game
    else if (choice == ' ') {
      if (pause_) {
        pause_ = false;
        audio_.Unpause();
        PrintMessage("Un-Paused game.", false);
      }
      else {
        pause_ = true;
        audio_.Pause();
        PrintMessage("Paused game.", false);
      }
    }

    else if (pause_) {
      continue;
    }

    else if (choice == 'h') {
      pause_ = true;
      PrintCentered(utils::LoadHelp());
      pause_ = false;
    }

    else if (choice == 'c') {
      for (int i=0; i<10; i++)
        player_one_->IncreaseResources(true);
      player_one_->set_iron(player_one_->iron()+1);
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
        Position nucleus_pos = player_one_->nucleus_pos();
        if (player_one_->GetAllPositionsOfNeurons(UnitsTech::NUCLEUS).size() > 1)
          nucleus_pos = SelectNeuron(player_one_, UnitsTech::NUCLEUS);
        if (nucleus_pos.first == -1)
          PrintMessage("Invalid choice!", true);
        else {
          PrintMessage("User the arrow keys to select a position. Press Enter to select.", false);
          Position pos = SelectPosition(nucleus_pos, player_one_->cur_range());
          if (pos.first != -1) {
            Position epsp_target= player_two_->nucleus_pos();
            Position ipsp_target = player_two_->GetRandomNeuron(); // random tower.
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
        Position nucleus_pos = player_one_->nucleus_pos();
        if (player_one_->GetAllPositionsOfNeurons(UnitsTech::NUCLEUS).size() > 1)
          nucleus_pos = SelectNeuron(player_one_, UnitsTech::NUCLEUS);
        if (nucleus_pos.first == -1)
          PrintMessage("Invalid choice!", true);
        else {
          PrintMessage("User the arrow keys to select a position. Press Enter to select.", false);
          Position pos = SelectPosition(nucleus_pos, player_one_->cur_range());
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
      std::string res = CheckMissingResources(player_one_->GetMissingResources(UnitsTech::NUCLEUS,
            player_one_->GetAllPositionsOfNeurons(UnitsTech::NUCLEUS).size()));
      if (res != "") 
        PrintMessage(res, true);
      else {
        PrintMessage("User the arrow keys to select a position. Press Enter to select.", false);
        Position pos = SelectPosition(player_one_->nucleus_pos(), ViewRange::GRAPH);
        if (pos.first != -1) {
          player_one_->AddNeuron(pos, UnitsTech::NUCLEUS);
          field_->AddNewUnitToPos(pos, UnitsTech::NUCLEUS);
        }
        PrintMessage(res, res!="");
      }
    }

    // T: Technology
    else if (choice == 'T') {
      pause_ = true;
      choice_mapping_t mapping;
      for (const auto& it : player_one_->technologies()) {
        size_t color = COLOR_DEFAULT;
        size_t missing_costs = player_one_->GetMissingResources(it.first, it.second.first+1).size();
        if (it.second.first < it.second.second && missing_costs == 0)
          color = COLOR_AVAILIBLE;
        mapping[it.first-UnitsTech::IPSP] = {units_tech_mapping.at(it.first) 
          + " (" + utils::PositionToString(it.second) + ")", color};
      }
      int technology = SelectInteger("Select technology", true, mapping, {3, 6, 9, 11})
        +UnitsTech::IPSP;
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
      Position pos = all_synapse_position.front(); 
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
          if (choice == 5)
            player_one_->SwitchSwarmAttack(pos);
          // Otherwise get new postion first.
          else if (choice == 1 || choice == 2) {
            auto start_position = SelectFieldPositionByAlpha(field_->GetAllCenterPositionsOfSections(), "Select start");
            if (start_position.first != -1) {
              auto new_pos = SelectPosition(start_position, ViewRange::GRAPH);
              if (new_pos.first != -1) {
                if (choice == 1)
                  player_one_->ResetWayForSynapse(pos, new_pos);
                else if (choice == 2)
                  player_one_->AddWayPosForSynapse(pos, new_pos);
              }
            }
          }
          else {
            auto new_pos = SelectPosition(player_two_->nucleus_pos(), ViewRange::GRAPH);
            if (new_pos.first != -1) {
              if (choice == 3)
                player_one_->ChangeIpspTargetForSynapse(pos, new_pos);
              else if (choice == 4)
                player_one_->ChangeEpspTargetForSynapse(pos, new_pos);
            }
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

Position Game::SelectPosition(Position start, int range) {
  bool end = false;
  Position new_pos = {-1, -1};
  field_->set_highlight({start});
  field_->set_range(range);
  field_->set_range_center(start);
  
  while(!game_over_ && !end) {
    int choice = getch();
    new_pos = field_->highlight().front();

    switch (choice) {
      case 10:
        if (field_->IsFree(new_pos) || range == ViewRange::GRAPH)
          end = true;
        else 
          PrintMessage("Invalid position (not free)!", false); 
        break;
      case 'q':
        end = true;
        new_pos = {-1, -1};
        break;
      case KEY_UP:
        new_pos.first--; 
        PrintFieldAndStatus();
        break;
      case KEY_DOWN:
        new_pos.first++; 
        PrintFieldAndStatus();
        break;
      case KEY_LEFT:
        new_pos.second--;
        PrintFieldAndStatus();
        break;
      case KEY_RIGHT:
        new_pos.second++;
        break;
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

Position Game::SelectNeuron(Player* p, int type) {
  std::string msg = "Choose " + units_tech_mapping.at(type) + ": ";
  return SelectFieldPositionByAlpha(p->GetAllPositionsOfNeurons(type), msg);
}

Position Game::SelectFieldPositionByAlpha(std::vector<Position> positions, std::string msg) {
  std::map<Position, char> replacements;
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
  pause_ = true;
  ClearField();
  bool end = false;
  // Get iron and print options.
  PrintCentered(LINES/2-1, "(use space j(←) and k(→) to circle through resources)");
  std::vector<std::string> symbols = {"O", SYMBOL_POTASSIUM, SYMBOL_SEROTONIN, 
    SYMBOL_GLUTAMATE, SYMBOL_DOPAMINE, SYMBOL_CHLORIDE};
  Position c = {LINES/2, COLS/2};
  std::vector<Position> positions = { {c.first-10, c.second}, {c.first-6, c.second+15}, {c.first+6, c.second+15}, 
    {c.first+10, c.second}, {c.first+6, c.second-15}, {c.first-6, c.second-15}, };

  unsigned int current = 0;
  while(!end) {
    std::string msg = "You can distribute " + std::to_string(player_one_->iron()) + " iron (FE).";
    PrintCentered(LINES/2-2, msg);
    std::string current_symbol = symbols[current];
    if (current_symbol == "O")
      msg = "oxygen. Costs: FE, current: " + std::to_string(player_one_->oxygen_boast());
    else {
      int resource = resources_symbol_mapping.at(current_symbol);
      msg = resources_name_mapping.at(resource) + ". Costs FE2. current: " + 
        ((player_one_->resources().at(resource).second) ? "active" : "inactive");
    }
    PrintCentered(LINES/2+1, "                                         ");
    PrintCentered(LINES/2+1, msg.c_str());

    for (unsigned int i=0; i<symbols.size(); i++) {
      if (symbols[i]== "O")
        attron(COLOR_PAIR(COLOR_SUCCESS));
      else if (player_one_->resources().at(resources_symbol_mapping.at(symbols[i])).second)
        attron(COLOR_PAIR(COLOR_SUCCESS));
      if (i == current)
        attron(COLOR_PAIR(COLOR_MARKED));
      mvaddstr(positions[i].first, positions[i].second, symbols[i].c_str());
      attron(COLOR_PAIR(COLOR_DEFAULT));
      refresh();
    }

    // Get players choice.
    char choice = getch();

    if (choice == 'k')
      current = (current+1)%symbols.size();
    else if (choice == 'j') {
      int n = current-1;
      int m = symbols.size();
      current = ((n%m)+m)%m;
    }
    else if (choice == 'q')
      end = true;
    else if (std::to_string(choice) == "10") {
      int resource = (resources_symbol_mapping.count(current_symbol) > 0) 
        ? resources_symbol_mapping.at(current_symbol) : Resources::OXYGEN;
      if (!player_one_->DistributeIron(resource)) {
        attron(COLOR_ERROR);
        PrintCentered(LINES/2+2, "Not enough iron!");
      }
      else {
        attron(COLOR_SUCCESS);
        PrintCentered(LINES/2+2, "    Selected!    ");
      }
    }
    if (player_one_->iron() == 0)
      end = true;
  }
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
    else if (mapping.count(int_choice) > 0 && (mapping.at(int_choice).second == COLOR_AVAILIBLE || !omit)) {
      pause_ = false;
      spdlog::get(LOGGER)->debug("Game::SelectInteger: done, retuning: {}", int_choice);
      return int_choice;
    }
    else if (mapping.count(int_choice) > 0 && mapping.at(int_choice).second != COLOR_AVAILIBLE && omit)
      PrintCentered(LINES/2+counter+5, "Selection not available (not enough resources?): " + std::to_string(int_choice));
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
  msg.insert(msg.length(), field_->cols()*2-msg.length(), char(46));
  std::unique_lock ul(mutex_print_field_);
  mvaddstr(LINE_MSG, left_border_+10, msg.c_str());
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
    mvaddstr(10+i, left_border_ + cols_*2 + 2, lines[i].c_str());
  }

  std::string msg = "Enemy potential: (" + player_two_->GetNucleusLive() + ")";
  PrintCentered(2, msg.c_str());
  msg = "Enemy resources: ";
  for (const auto& it : player_two_->resources()) 
    msg += resources_name_mapping.at(it.first) + ": " + std::to_string(it.second.first) + ", ";
  PrintCentered(3, msg.c_str());
  msg = "Enemy technologies: ";
  for (const auto& it : player_two_->technologies()) 
    msg += std::to_string(it.first) + ": " + std::to_string(it.second.first) + ", ";
  PrintCentered(4, msg.c_str());
  refresh();
}

void Game::SetGameOver(std::string msg) {
  std::unique_lock ul(mutex_print_field_);
  clear();
  PrintCentered(LINES/2, msg);
  refresh();
  game_over_ = true;
}

void Game::PrintCentered(Paragraphs paragraphs) {
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
  parts.push_back({"[D]istribute iron", player_one_->resources().at(Resources::IRON).first > 0});
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
