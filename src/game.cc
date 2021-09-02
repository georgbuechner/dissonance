#include "game.h"
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <curses.h>
#include <exception>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <time.h>
#include <thread>
#include <vector>

#include "codes.h"
#include "player.h"
#include "units.h"
#include "utils.h"
#include "ki.h"

#define HELP "[e]psp, [i]psp | [A]ctivate neuron, build [S]ynapse | [d]istribute iron, [s]elect synapse | [h]elp | pause [space], [q]uit"
#define COLOR_DEFAULT 3
#define COLOR_ERROR 2 
#define COLOR_MSG 4

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

Game::Game(int lines, int cols) : game_over_(false), pause_(false) {
  field_ = new Field(lines, cols);
  field_->AddHills();
  player_one_ = new Player(field_->AddDen(lines/1.5, lines-5, cols/1.5, cols-5), 3);
  player_two_ = new Player(field_->AddDen(5, lines/3, 5, cols/3), 4);
  field_->BuildGraph(player_one_->nucleus_pos(), player_two_->nucleus_pos());
}

void Game::play() {
  // Print welcome text.
  PrintCentered(utils::LoadWelcome());

  // Select difficulty
  difficulty_ = SelectInteger("Select difficulty", false, {1, 2, 3, 4, 5, 6, 7, 8, 9});

  DistributeIron();

  std::thread thread_actions([this]() { DoActions(); });
  std::thread thread_choices([this]() { (GetPlayerChoice()); });
  std::thread thread_ki([this]() { (HandleKi()); });
  thread_actions.join();
  thread_choices.join();
  thread_ki.join();
}

void Game::DoActions() {
  auto last_update = std::chrono::steady_clock::now();
  auto last_resource_player_one = std::chrono::steady_clock::now();
  auto last_resource_player_two = std::chrono::steady_clock::now();

  int ki_update_frequency = RESOURCE_UPDATE_FREQUENCY;
  ki_update_frequency -= 40*(difficulty_-2);
  PrintCentered(1, "difficulty: " + std::to_string(difficulty_));
 
  while (!game_over_) {
    auto cur_time = std::chrono::steady_clock::now();

    if (pause_) continue;
    
    // Increase resources.
    if (utils::get_elapsed(last_resource_player_one, cur_time) > RESOURCE_UPDATE_FREQUENCY) {
      player_one_->IncreaseResources();
      last_resource_player_one = cur_time;
    }

    if (utils::get_elapsed(last_resource_player_two, cur_time) > ki_update_frequency) {
      player_two_->IncreaseResources();
      last_resource_player_two = cur_time;
    }

    if (utils::get_elapsed(last_update, cur_time) > UPDATE_FREQUENCY) {
      // Move player soldiers and check if enemy den's lp is down to 0.
      player_one_->MovePotential(player_two_);
      if (player_two_->HasLost()) {
        SetGameOver("YOU WON");
        break;
      }
      player_two_->MovePotential(player_one_);
      if (player_one_->HasLost()) {
        SetGameOver("YOU LOST");
        break;
      }

      // Remove enemy soldiers in renage of defence towers.
      player_one_->HandleDef(player_two_);
      player_two_->HandleDef(player_one_);

      // Refresh page
      PrintFieldAndStatus();
      last_update = cur_time;
    }
  } 
}

void Game::HandleKi() {
  Ki* ki = new Ki(player_one_, player_two_);
  ki->SetUpKi(difficulty_);

  // Handle building neurons and potentials.
  while(!game_over_) {
    if (!pause_)
      ki->UpdateKi(field_);
  }
}

void Game::GetPlayerChoice() {
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
    
    // SPACE: pause/ unpause game
    else if (choice == ' ') {
      if (pause_) {
        pause_ = false;
        PrintMessage("Un-Paused game.", false);
      }
      else {
        pause_ = true;
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
        player_one_->IncreaseResources();
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
          player_one_->AddPotential(pos, field_, UnitsTech::EPSP);
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
          PrintMessage("created isps @synapse: " + utils::PositionToString(pos), false);
          player_one_->AddPotential(pos, field_, UnitsTech::IPSP);
        }
      }
    }

    // S: Synapse
    else if (choice == 'S') {
      std::string res = CheckMissingResources(player_one_->GetMissingResources(UnitsTech::SYNAPSE));
      if (res != "") 
        PrintMessage(res, true);
      else {
        Position nucleus_pos = SelectNeuron(player_one_, UnitsTech::NUCLEUS);
        if (nucleus_pos.first == -1)
          PrintMessage("Invalid choice!", true);
        else {
          PrintMessage("User the arrow keys to select a position. Press Enter to select.", false);
          Position pos = SelectPosition(nucleus_pos, player_one_->cur_range());
          if (pos.first != -1) {
            Position epsp_target= player_two_->nucleus_pos();
            Position ipsp_target = player_two_->activated_neurons().begin()->second.pos_; // random tower.
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
        Position nucleus_pos = SelectNeuron(player_one_, UnitsTech::NUCLEUS);
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
            player_one_->all_nucleus().size()));
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
      std::vector<int> options;
      std::map<int, std::string> mapping;
      for (const auto& it : player_one_->technologies()) {
        options.push_back(it.first-UnitsTech::NUCLEUS);
        mapping[it.first-UnitsTech::NUCLEUS] += units_tech_mapping.at(it.first) 
          + " (" + utils::PositionToString(it.second) + ")";
      }
      int technology = SelectInteger("Select technology", true, options, mapping, {4, 7, 10, 12})
        +UnitsTech::NUCLEUS;
      if (player_one_->AddTechnology(technology))
        PrintMessage("selected: " + units_tech_mapping.at(technology), false);
      else if (technology != -1)
        PrintMessage("Not enough resources or inavlid selection", true);
      pause_ = false;
    }

    else if (choice == 's') {
      auto pos = SelectNeuron(player_one_, UnitsTech::SYNAPSE);
      if (pos.first == -1)
        PrintMessage("Invalid choice!", true);
      else {
        Synapse synapse = player_one_->GetSynapse(pos);

        // Create options.
        std::vector<int> options;
        std::map<int, int> mapping_option_to_func;
        std::map<int, std::string> mapping_option_to_desc;
        int counter = 0;
        
        // Select (new) way/ Add way-point.
        if (player_one_->technologies().at(UnitsTech::WAY).first > 0) {
          options.push_back(++counter);
          mapping_option_to_desc[counter] = "Select way.";
          mapping_option_to_func[counter] = 1;
          if (synapse.ways_.size() < synapse.availible_ways_) {
            options.push_back(++counter);
            mapping_option_to_desc[counter] = "Add way-point";
            mapping_option_to_func[counter] = 2;
          }
        }

        // Select target ipsp/ epsp
        if (player_one_->technologies().at(UnitsTech::TARGET).first > 0) {
          options.push_back(++counter);
          mapping_option_to_desc[counter] = "Select target for ipsp.";
          mapping_option_to_func[counter] = 3;
        }
        if (player_one_->technologies().at(UnitsTech::TARGET).first > 1) {
          options.push_back(++counter);
          mapping_option_to_desc[counter] = "Select target for epsp.";
          mapping_option_to_func[counter] = 4;
        }

        // Switch swarm on/ off
        int swarm = player_one_->technologies().at(UnitsTech::SWARM).first;
        if (swarm > 0) {
          options.push_back(++counter);
          if (synapse.swarm_)
            mapping_option_to_desc[counter] = "Turn swarm-attack off";
          else 
            mapping_option_to_desc[counter] = "Turn swarm-attack on";
          mapping_option_to_func[counter] = 5;
          mapping_option_to_desc[counter] += " (currently " + std::to_string(synapse.max_stored_) + ").";
        }

        int choice = SelectInteger("What to do?", true, options, mapping_option_to_desc);
        if (mapping_option_to_func.count(choice) > 0) {
          if (mapping_option_to_func[choice] == 1) {
            auto new_way_point = SelectPosition(player_two_->nucleus_pos(), ViewRange::GRAPH);
            player_one_->ResetWayForSynapse(pos, new_way_point);
          }
          else if (mapping_option_to_func[choice] == 2) {
            auto new_way_point = SelectPosition(player_two_->nucleus_pos(), ViewRange::GRAPH);
            player_one_->AddWayPosForSynapse(pos, new_way_point);
          }
          else if (mapping_option_to_func[choice] == 3) {
            auto new_target_pos = SelectPosition(player_two_->nucleus_pos(), ViewRange::GRAPH);
            player_one_->ChangeIpspTargetForSynapse(pos, new_target_pos);
          }
          else if (mapping_option_to_func[choice] == 4) {
            auto new_target_pos = SelectPosition(player_two_->nucleus_pos(), ViewRange::GRAPH);
            player_one_->ChangeEpspTargetForSynapse(pos, new_target_pos);
          }
          else if (mapping_option_to_func[choice] == 5) {
            player_one_->SwitchSwarmAttack(pos);
          }
          PrintMessage("Selected choice: " + std::to_string(choice), false);
        }
        else {
          PrintMessage("Invalid choice: " + std::to_string(choice), true);
        }
      }
    }

    else if (choice == 'd') {
      pause_ = true;
      DistributeIron();
      pause_= false; 
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
        new_pos = {0, 0};
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
  // create replacements (map barack position to letter a..z).
  std::map<Position, char> replacements;
  int counter = 0;
  if (type == UnitsTech::SYNAPSE) {
    for (auto it : p->synapses()) 
      replacements[it.first] = (int)'a'+(counter++);
  }
  else if (type == UnitsTech::NUCLEUS) {
    for (auto it : p->all_nucleus()) 
      replacements[it.first] = (int)'a'+(counter++);
  }
  field_->set_replace(replacements);

  // Print field and get player choice and reset replacements.
  PrintFieldAndStatus();
  PrintMessage("Choose barack: ", false);
  char choice = getch();
  field_->set_replace({});

  // Find selected barack in replacements and return.
  for (auto it : replacements) {
    if (it.second == choice)
      return it.first;
  }
  return {-1, -1};
}

void Game::DistributeIron() {
  ClearField();
  bool end = false;
  while(!end) {
    // Get iron and print options.
    int iron = player_one_->iron();
    std::string msg = "You can distribute " + std::to_string(iron) + " iron.";
    PrintCentered(LINES/2-1, msg);

    auto resources = player_one_->resources();
    std::map<int, std::string> selection = {{Resources::OXYGEN, "oxygen-boast c=FE"}};
    for (const auto& it : resources) {
      if (it.first!= Resources::IRON && !it.second.second)
        selection[it.first] = resources_name_mapping.at(it.first) + " c=FE2";
    }
    std::string options = "";
    for (const auto& it : selection) {
      options += it.second + " (" + std::to_string(it.first) + ")    ";
    }
    PrintCentered(LINES/2+1, options);

    // Get players choice.
    char c = getch();
    PrintCentered(LINES/2+2, "selection: " + std::to_string(c-49));
    if (c == 'q')
      end = true;
    else {
      ClearField();
      if (!player_one_->DistributeIron(c-48))
        PrintCentered(LINES/2, "Invalid selection or not enough iron!");
    }

    if (player_one_->iron() == 0)
      end = true;
  }
}

int Game::SelectInteger(std::string msg, bool omit, 
    std::vector<int> options, std::map<int, std::string> mapping, std::vector<int> splits) {
  pause_ = true;
  ClearField();
  bool end = false;
  while (!end) {
    // Print options.
    std::string availible_options = "";
    for (const auto& option : options) {
      char c_option = 'a'+option-1;
      availible_options+= c_option;
      if (mapping.count(option) > 0)
        availible_options += ": " + mapping[option];
      availible_options+= "    ";
    }
    PrintCentered(LINES/2-1, msg);

    int counter = 1;
    for (const auto& split : splits) {
      std::string s_split;
      char c_split = 'a'+split-1;
      s_split += c_split;

      size_t pos = availible_options.find(s_split + ": ");
      PrintCentered(LINES/2+(counter+=2), availible_options.substr(0, pos));
      availible_options = availible_options.substr(pos);
    }
    PrintCentered(LINES/2+(counter+=2), availible_options);
    PrintCentered(LINES/2+counter+3, "> enter number...");

    // Get choice.
    char choice = getch();
    if (choice == 'q' && omit)
      end = true;
    else if (std::find(options.begin(), options.end(), choice-'a'+1) != options.end()) {
      pause_ = false;
      return choice-'a'+1;
    }
    else 
      PrintCentered(LINES/2+5, "Wrong selection: " + std::to_string(choice-'a'+1));
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
  mvaddstr(LINE_MSG, 10, msg.c_str());
  attron(COLOR_PAIR(COLOR_DEFAULT));
  refresh();
}

void Game::PrintFieldAndStatus() {
  std::unique_lock ul(mutex_print_field_);
  mvaddstr(LINE_HELP, 10, HELP);
  field_->PrintField(player_one_, player_two_);
  
  auto lines = player_one_->GetCurrentStatusLine();
  for (unsigned int i=0; i<lines.size(); i++) {
    mvaddstr(10+i, COLS-28, lines[i].c_str());
  }

  /*
  std::string msg = "Enemy nucleus potential " + std::to_string(player_two_->nucleus_potential()) + "/9";
  PrintCentered(2, msg.c_str());
  msg = "Enemy iron: " + std::to_string(player_two_->resources().at(Resources::IRON).first);
  PrintCentered(3, msg.c_str());
  msg = "Enemy oxygen: " + std::to_string(player_two_->resources().at(Resources::OXYGEN).first);
  PrintCentered(4, msg.c_str());
  msg = "Enemy potassium: " + std::to_string(player_two_->resources().at(Resources::POTASSIUM).first);
  PrintCentered(5, msg.c_str());
  msg = "Enemy glutamate: " + std::to_string(player_two_->resources().at(Resources::GLUTAMATE).first);
  PrintCentered(6, msg.c_str());
  */

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

void Game::ClearField() {
  std::unique_lock ul(mutex_print_field_);
  clear();
  refresh();
}
