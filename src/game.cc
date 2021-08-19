#include "game.h"
#include <chrono>
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

#define HELP "epsp [e], ipsp [i], tech [t] | activate neuron [A], build synapse [S] | Distribute Iron [E] | Help [h] | pause [space] quit [q]"
#define COLOR_DEFAULT 3
#define COLOR_ERROR 2 
#define COLOR_MSG 4

#define LINE_HELP 8
#define LINE_STATUS LINES-9
#define LINE_MSG LINES-6

#define RESOURCES_UPDATE_FREQUENCY 750
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
  auto welome = utils::LoadWelcome();
  for (const auto& paragraph : welome) {
    int size = paragraph.size()/2;
    int counter = 0;
    for (const auto& line : paragraph) {
      PrintCentered(LINES/2-size+(counter++), line);
    }
    PrintCentered(LINES/2+size+2, "[Press any key to continue]");
    char c = getch();
    c++;
    refresh();
    clear();
  }

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
  auto last_resource_inc = std::chrono::steady_clock::now();
 
  while (!game_over_) {
    auto cur_time = std::chrono::steady_clock::now();

    if (pause_) continue;
    
    // Increase resources.
    if (utils::get_elapsed(last_resource_inc, cur_time) > RESOURCES_UPDATE_FREQUENCY) {
      // Increase players resources.
      player_one_->IncreaseResources();
      last_resource_inc = cur_time;
    }

    if (utils::get_elapsed(last_update, cur_time) > UPDATE_FREQUENCY) {
      // Move player soldiers and check if enemy den's lp is down to 0.
      int damage = player_one_->MovePotential(player_two_->nucleus_pos(), player_two_);
      if (player_two_->IncreaseNeuronPotential(damage, Units::NUCLEUS)) {
        SetGameOver("YOU WON");
        break;
      }
      damage = player_two_->MovePotential(player_one_->nucleus_pos(), player_one_);
      if (player_one_->IncreaseNeuronPotential(damage, Units::NUCLEUS)) {
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
  Ki* ki = new Ki();

  player_two_->DistributeIron(Resources::OXYGEN);
  player_two_->DistributeIron(Resources::GLUTAMATE);
  for (int i=0; i<5; i++) 
    player_two_->IncreaseResources();

  while(!game_over_) {
    if (pause_) continue;

    auto cur_time = std::chrono::steady_clock::now();

    if (utils::get_elapsed(ki->last_def(), cur_time) > ki->new_tower_frequency()) {
      player_two_->IncreaseResources();
      ki->reset_last_def();
    }

    // if (utils::get_elapsed(ki->last_update(), cur_time) > ki->update_frequency()) 
    //   ki->reset_last_update();
    // else 
    //   continue;

    if (player_two_->CheckResources(Units::SYNAPSE).size() == 0 && player_two_->synapses().size() == 0) {
      auto pos = field_->FindFree(player_two_->nucleus_pos().first, player_two_->nucleus_pos().second, 1, 5);
      player_two_->AddNeuron(pos, Units::SYNAPSE);
      field_->AddNewUnitToPos(pos, Units::SYNAPSE);
    }

    if (player_two_->resources().at(Resources::POTASSIUM).first > ki->attacks().front() && player_two_->synapses().size() > 0) {
      auto synapse_pos = player_two_->synapses().begin()->first;
      while (player_two_->CheckResources(Units::EPSP).size() == 0) {
        auto cur_time_b = std::chrono::steady_clock::now(); 
        if (utils::get_elapsed(ki->last_soldier(), cur_time_b) > ki->new_soldier_frequency()) 
          ki->reset_last_soldier();
        else 
          continue;
        auto pos = field_->GetNewSoldierPos(synapse_pos);
        auto way = field_->GetWayForSoldier(synapse_pos, player_one_->nucleus_pos());
        player_two_->AddPotential(pos, way, Units::EPSP);
      }
      if (ki->attacks().size() > 1)
        ki->attacks().pop_front();
    }

    if (player_two_->CheckResources(Units::ACTIVATEDNEURON).size() == 0 
        && ki->max_towers() >= player_two_->activated_neurons().size()) {
      // Only add def, if a barrak already exists, or no def exists.
      if (!(player_two_->activated_neurons().size() > 0 && player_two_->synapses().size() == 0)) {
        auto pos = field_->FindFree(player_two_->nucleus_pos().first, player_two_->nucleus_pos().second, 1, 5);
        field_->AddNewUnitToPos(pos, Units::ACTIVATEDNEURON);
        player_two_->AddNeuron(pos, Units::ACTIVATEDNEURON);
      }
    }

    /*
    if (ki->attacks().size() == 1) {
      ki->set_max_towers(ki->max_towers()+2);
      ki->update_frequencies();
    }
    else if (ki->attacks().size() == 2) {
      player_two_->set_resource_curve(player_two_->resource_curve()-1);
    }
    */

    if (player_two_->iron() > 0) {
      if (player_two_->IsActivatedResource(Resources::POTASSIUM))
        player_two_->DistributeIron(Resources::IRON);
      else if (player_two_->iron() == 2)
        player_two_->DistributeIron(Resources::POTASSIUM);
    }
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

    else if (choice == 'h') {
      pause_ = true;
      refresh();
      clear();
      auto help = utils::LoadHelp();
      for (const auto& paragraph : help) {
        int size = paragraph.size()/2;
        int counter = 0;
        for (const auto& line : paragraph) {
          PrintCentered(LINES/2-size+(counter++), line);
        }
        PrintCentered(LINES/2+size+2, "[Press any key to continue]");
        char c = getch();
        c++;
        refresh();
        clear();
      }
      pause_ = false;
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
    // e: add epsp
    else if (choice == 'e') {
      std::string res = CheckMissingResources(player_one_->CheckResources(Units::EPSP));
      if (res != "")
        PrintMessage(res, true);
      else {
        auto pos = SelectBarack(player_one_);
        if (pos.first == -1)
          PrintMessage("Invalid choice!", true);
        else {
          PrintMessage("Added epsp at synapse @" + utils::PositionToString(pos), false);
          std::list<Position> way = field_->GetWayForSoldier(pos, player_two_->nucleus_pos());
          player_one_->AddPotential(pos, way, Units::EPSP);
        }
      }
    }
    // i: add ipsp 
    else if (choice == 'i') {
      std::string res = CheckMissingResources(player_one_->CheckResources(Units::IPSP));
      if (res != "")
        PrintMessage(res, true);
      else {
        auto pos = SelectBarack(player_one_);
        if (pos.first == -1)
          PrintMessage("Invalid choice!", true);
        else {
          // PrintMessage("Added ibsp at synapse @" + utils::PositionToString(pos), false);
          Position target = player_two_->activated_neurons().begin()->second.pos_;
          // PrintMessage("Added ibsp with target: " + utils::PositionToString(target), false);
          std::list<Position> way = field_->GetWayForSoldier(pos, target);
          PrintMessage("way size : " + std::to_string(way.size()), false);
          player_one_->AddPotential(pos, way, Units::IPSP);
        }
      }
    }

    // S: Synapse
    else if (choice == 'S') {
      std::string res = CheckMissingResources(player_one_->CheckResources(Units::SYNAPSE));
      PrintMessage("User the arrow keys to select a position. Press Enter to select.", false);
      if (res != "") 
        PrintMessage(res, true);
      else {
        Position pos = SelectPosition(player_one_->nucleus_pos(), player_one_->cur_range());
        if (pos.first != -1) {
          player_one_->AddNeuron(pos, Units::SYNAPSE);
          field_->AddNewUnitToPos(pos, Units::SYNAPSE);
        }
        PrintMessage(res, res!="");
      }
    }

    // D: place defence-tower
    else if (choice == 'A') {
      std::string res = CheckMissingResources(player_one_->CheckResources(Units::ACTIVATEDNEURON));
      PrintMessage("User the arrow keys to select a position. Press Enter to select.", false);
      if (res != "") 
        PrintMessage(res, true);
      else {
        Position pos = SelectPosition(player_one_->nucleus_pos(), player_one_->cur_range());
        if (pos.first != -1) {
          player_one_->AddNeuron(pos, Units::ACTIVATEDNEURON);
          field_->AddNewUnitToPos(pos, Units::ACTIVATEDNEURON);
        }
        PrintMessage(res, res!="");
      }
    }

    else if (choice == 'E') {
      pause_ = true;
      DistributeIron();
      pause_= false; 
    }
  } 
}

Position Game::SelectPosition(Position start, int range) {
  bool end = false;
  int choice;
  Position new_pos = {0, 0};
  field_->set_highlight({start});
  field_->set_range(range);
  
  while(!game_over_ && !end) {
    choice = getch();
    new_pos = field_->highlight().front();

    switch (choice) {
      case 10:
        if (field_->IsFree(new_pos))
          end = true;
        else 
          PrintMessage("Invalid position (not free)!", false); 
        break;
      case 'q':
        end = true;
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
    if (field_->InRange(new_pos, range, player_one_->nucleus_pos())) {
      field_->set_highlight({new_pos});
      PrintFieldAndStatus();
    }
    else 
      PrintMessage("Outside of range!", false); 
  }
  field_->set_range(ViewRange::HIDE);
  field_->set_highlight({});
  return new_pos;
}

Position Game::SelectBarack(Player* p) {
  // create replacements (map barack position to letter a..z).
  std::map<Position, char> replacements;
  int counter = 0;
  for (auto it : p->synapses()) 
    replacements[it.first] = (int)'a'+(counter++);
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
  clear();
  refresh();

  bool end = false;
  while(!end) {
    int iron = player_one_->iron();
    std::string msg = "You can distribute " + std::to_string(iron) + " iron.";

    PrintCentered(LINES/2-1, msg);

    auto resources = player_one_->resources();
    std::map<int, std::string> selection = {{Resources::OXYGEN, "oxygen-boast"}};
    for (const auto& it : resources) {
      if (it.first!= Resources::IRON && !it.second.second)
        selection[it.first] = resources_name_mapping.at(it.first);
    }
    std::string txt = "";
    for (const auto& it : selection) {
      txt += it.second + "(" + std::to_string(it.first) + ")    ";
    }
    PrintCentered(LINES/2+1, txt);
    char c = getch();
    PrintCentered(LINES/2+2, "selection: " + std::to_string(c-49));
    if (c == 'q')
      end = true;
    else {
      if (!player_one_->DistributeIron(c-48)) {
        clear();
        refresh();
        PrintCentered(LINES/2, "Invalid selection or not enough iron!");
      }
      else {
        clear();
        refresh();
      }
    }

    if (player_one_->iron() == 0) {
      end = true;
    }
  }
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
  PrintCentered(LINE_STATUS, player_one_->GetCurrentStatusLineA().c_str());
  PrintCentered(LINE_STATUS+1, player_one_->GetCurrentStatusLineB().c_str());
  PrintCentered(LINE_STATUS+2, player_one_->GetCurrentStatusLineC().c_str());

  // std::string msg = "Enemy iron: " + std::to_string(player_two_->resources().at(Resources::IRON).first);
  // PrintCentered(3, msg.c_str());
  // msg = "Enemy oxygen: " + std::to_string(player_two_->resources().at(Resources::OXYGEN).first);
  // PrintCentered(4, msg.c_str());
  // msg = "Enemy potassium: " + std::to_string(player_two_->resources().at(Resources::POTASSIUM).first);
  // PrintCentered(5, msg.c_str());
  // msg = "Enemy glutamate: " + std::to_string(player_two_->resources().at(Resources::GLUTAMATE).first);
  // PrintCentered(6, msg.c_str());

  refresh();
}

void Game::SetGameOver(std::string msg) {
  clear();
  PrintCentered(LINES/2, msg);
  refresh();
  game_over_ = true;
}

void Game::PrintCentered(int line, std::string txt) {
  mvaddstr(line, COLS/2-txt.length()/2, txt.c_str());
}
