#include "game.h"
#include <chrono>
#include <cstdlib>
#include <curses.h>
#include <exception>
#include <mutex>
#include <time.h>
#include <thread>
#include <vector>

#include "utils.h"
#include "ki.h"

#define HELP "soldier [a], gatherer [r], tech [t] | barracks [A], resources [R], defence [D] tech [T] | pause [space] quit [q]"
#define DEFAULT_COLORS 3
#define ERROR 2 
#define MSG 4

#define LINE_HELP 8
#define LINE_STATUS LINES-9
#define LINE_MSG LINES-8

#define RESOURCES_UPDATE_FREQUENCY 750
#define UPDATE_FREQUENCY 200 

#define KI_NEW_SOLDIER 1250
#define KI_NEW_SOLDIER 1250

Game::Game(int lines, int cols) {
  field_ = new Field(lines, cols);
  field_->AddHills();
  player_one_ = new Player(field_->AddDen(lines/1.5, lines-5, cols/1.5, cols-5), 4);
  player_two_ = new Player(field_->AddDen(5, lines/3, 5, cols/3), 100000);
  field_->BuildGraph(player_one_->den_pos(), player_two_->den_pos());

  game_over_ = false;
}

void Game::play() {
  std::thread thread_actions([this]() { DoActions(); });
  std::thread thread_choices([this]() { (GetPlayerChoice()); });
  std::thread thread_ki([this]() { (HandleKi()); });
  thread_actions.join();
  thread_choices.join();
}

void Game::DoActions() {
  auto last_update = std::chrono::steady_clock::now();
  auto last_resource_inc = std::chrono::steady_clock::now();
 
  while (!game_over_) {
    auto cur_time = std::chrono::steady_clock::now();
    
    // Increase resources.
    if (utils::get_elapsed(last_resource_inc, cur_time) > RESOURCES_UPDATE_FREQUENCY) {
      // Increase players resources.
      player_one_->IncreaseResources();
      last_resource_inc = cur_time;
    }

    if (utils::get_elapsed(last_update, cur_time) > UPDATE_FREQUENCY) {
      // Move player soldiers and check if enemy den's lp is down to 0.
      int damage = player_one_->MoveSoldiers(player_two_->den_pos());
      if (player_two_->DecreaseDenLp(damage)) {
        SetGameOver("YOU WON");
        break;
      }
      damage = player_two_->MoveSoldiers(player_one_->den_pos());
      if (player_one_->DecreaseDenLp(damage)) {
        SetGameOver("YOU LOST");
        break;
      }

      // Remove enemy soldiers in renage of defence towers.
      player_one_->HandleDef(player_two_);
      player_two_->HandleDef(player_one_);

      // Refresh page
      PrintFieldAndStatus();
      refresh();
      last_update = cur_time;
    }
  } 
}

void Game::HandleKi() {
  Ki* ki = new Ki();
  while(!game_over_) {
    auto cur_time = std::chrono::steady_clock::now();
    if (utils::get_elapsed(ki->last_soldier(), cur_time) > ki->new_soldier_frequency()) {
      auto pos = field_->GetNewSoldierPos(player_two_->den_pos());
      auto way = field_->GetWayForSoldier(pos, player_one_->den_pos());
      std::string str = player_two_->AddSoldier(pos, way);
      ki->reset_last_soldier();
    }

    if (utils::get_elapsed(ki->last_def(), cur_time) > ki->new_tower_frequency()) {
      field_->GetNewDefencePos(player_two_);
      ki->reset_last_def();
    }

    if (utils::get_elapsed(ki->last_update(), cur_time) > ki->update_frequency()) {
      ki->update_frequencies();
      ki->reset_last_update();
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

    // r: add gatherer 
    else if (choice == 'r') {
      PrintMessage("gold-, bronze- or silver-gatherer ([g]/[b]/[s]", false);
      // Select bronze, silver or gold gatherer.
      int add_choice = getch();
      std::string res = "";
      if (add_choice == 'b')
        res = player_one_->AddGathererBronze();
      else if (add_choice == 's')
        res = player_one_->AddGathererSilver();
      else if (add_choice == 'g')
        res = player_one_->AddGathererGold();
      // Use error color in case of error.
      PrintMessage(res, res!="");
    }
    // a: add soldier
    else if (choice == 'a') {
      auto pos = field_->GetNewSoldierPos(player_one_->den_pos());
      std::list<Position> way = field_->GetWayForSoldier(pos, player_two_->den_pos());
      std::string res = player_one_->AddSoldier(pos, way);
      PrintMessage(res, res != "");
    }
    // D: place defence-tower
    else if (choice == 'D') {
      std::string res = player_one_->CheckDefenceTowerResources();
      if (res == "")
        player_one_->AddDefenceTower(field_->GetNewDefencePos(player_one_));
      PrintMessage(res, res!="");
    }
  } 
}

void Game::PrintMessage(std::string msg, bool error) {
  if (error)
    attron(COLOR_PAIR(ERROR));
  else
    attron(COLOR_PAIR(MSG));
  msg.insert(msg.length(), field_->cols()*2-msg.length(), char(46));
  std::unique_lock ul(mutex_print_field_);
  mvaddstr(LINE_MSG, 10, msg.c_str());
  attron(COLOR_PAIR(DEFAULT_COLORS));
  refresh();
}

void Game::PrintFieldAndStatus() {
  std::unique_lock ul(mutex_print_field_);
  mvaddstr(LINE_HELP, 10, HELP);
  field_->PrintField(player_one_, player_two_);
  mvaddstr(LINE_STATUS, 10, player_one_->GetCurrentStatusLine().c_str());
}

void Game::SetGameOver(std::string msg) {
  clear();
  mvaddstr(LINES/2, COLS/2-4, msg.c_str());
  refresh();
  game_over_ = true;
}
