#include "game.h"
#include <chrono>
#include <cstdlib>
#include <curses.h>
#include <exception>
#include <mutex>
#include <time.h>
#include <thread>
#include <vector>
#include "ki.h"

#define HELP "soldier [a], gatherer [r], tech [t] | barracks [A], resources [R], defence [D] tech [T] | pause [space] quit [q]"
#define DEFAULT_COLORS 3
#define ERROR 2 
#define MSG 4

#define LINE_HELP 8
#define LINE_STATUS LINES-9
#define LINE_MSG LINES-8

#define RESOURCES_UPDATE_FREQUENCY 750
#define SOLDIER_MOVEMENT_FREQUENCY 500
#define DEFENCE_UPDATE_FREQUENCY 800 

#define KI_NEW_SOLDIER 1250
#define KI_NEW_SOLDIER 1250

double get_elapsed(std::chrono::time_point<std::chrono::steady_clock> start,
    std::chrono::time_point<std::chrono::steady_clock> end) {
  return std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
}

Game::Game(int lines, int cols) {
  field_ = new Field(lines, cols);
  field_->add_hills();
  auto player_den_pos = field_->add_player();
  auto ki_den_pos = field_->add_ki();
  field_->build_graph();

  player_ = new Player(player_den_pos, 4);
  ki_ = new Player(ki_den_pos, 1000);

  game_over_ = false;
}

void Game::play() {
  std::thread thread_actions([this]() { do_actions(); });
  std::thread thread_choices([this]() { (get_player_choice()); });
  thread_actions.join();
  thread_choices.join();
}

void Game::do_actions() {
  auto last_soldier_movement = std::chrono::steady_clock::now();
  auto last_resource_inc = std::chrono::steady_clock::now();
  auto last_defence_update = std::chrono::steady_clock::now();
  Ki* ki = new Ki();
 
  bool refresh_screen = false;
  while (true) {
    auto cur_time = std::chrono::steady_clock::now();
    refresh_screen = false;
    // Increase resources.
    if (get_elapsed(last_resource_inc, cur_time) > RESOURCES_UPDATE_FREQUENCY) {
      // Increase players resources.
      player_->inc();
      last_resource_inc = cur_time;
      refresh_screen = true;
    }

    if (get_elapsed(last_soldier_movement, cur_time) > SOLDIER_MOVEMENT_FREQUENCY) {
      // Move player soldiers and check if enemy den's lp is down to 0.
      int life = player_->update_soldiers(ki_->den_pos());
      if (ki_->decrease_den_lp(life)) {
        clear();
        mvaddstr(LINES/2, COLS/2-4, "YOU WON");
        refresh();
        game_over_ = true;
        break;
      }
      life = ki_->update_soldiers(player_->den_pos());
      if (player_->decrease_den_lp(life)) {
        clear();
        mvaddstr(LINES/2, COLS/2-4, "YOU LOST");
        refresh();
        game_over_ = true;
        break;
      }

      last_soldier_movement = cur_time;
      refresh_screen = true;
    }

    if (get_elapsed(last_defence_update, cur_time) > DEFENCE_UPDATE_FREQUENCY) {
      field_->handle_def(player_, ki_);
      last_defence_update = cur_time;
      refresh_screen = true;
    }

    // KI 
    
    if (get_elapsed(ki->last_soldier(), cur_time) > ki->new_soldier_frequency()) {
      auto pos = field_->get_new_soldier_pos(false);
      auto way = field_->get_way_for_soldier(pos, false);
      std::string str = ki_->add_soldier(pos, way);
      ki->reset_last_soldier();
      refresh_screen = true;
    }

    if (get_elapsed(ki->last_def(), cur_time) > ki->new_tower_frequency()) {
      field_->get_new_defence_pos(false);
      ki->reset_last_def();
      refresh_screen = true;
    }

    if (get_elapsed(ki->last_update(), cur_time) > ki->update_frequency()) {
      ki->update_frequencies();
      ki->reset_last_update();
    }

    if (refresh_screen) {
      // Refresh and print field.
      print_field();
      refresh();
    }
  } 
}

void Game::get_player_choice() {
  int choice;
  std::string str = "";
  print_field();
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
      std::unique_lock ul(mutex_print_field_);
      attron(COLOR_PAIR(MSG));
      mvaddstr(LINE_MSG, 10, "gold-, bronze- or silver-gatherer ([g]/[b]/[s])");
      ul.unlock();
      attron(COLOR_PAIR(DEFAULT_COLORS));
      // Select bronze, silver or gold gatherer.
      int add_choice = getch();
      if (add_choice == 'b')
        str = player_->add_gatherer_bronze();
      else if (add_choice == 's')
        str = player_->add_gatherer_silver();
      else if (add_choice == 'g')
        str = player_->add_gatherer_gold();
      // Use error color in case of error.
      if (str != "")
        attron(COLOR_PAIR(ERROR));
      str.insert(str.length(), field_->cols()*2-str.length(), char(46));
      ul.lock();
      mvaddstr(LINE_MSG, 10, str.c_str());
      attron(COLOR_PAIR(DEFAULT_COLORS));
    }
    // a: add soldier
    else if (choice == 'a') {
      auto pos = field_->get_new_soldier_pos(true);
      std::list<std::pair<int, int>> way = {};
      way = field_->get_way_for_soldier(pos, true);
      str = player_->add_soldier(pos, way);
      if (str != "")
        attron(COLOR_PAIR(ERROR));
      str.insert(str.length(), field_->cols()*2-str.length(), char(46));
      std::unique_lock ul(mutex_print_field_);
      mvaddstr(LINE_MSG, 10, str.c_str());
      attron(COLOR_PAIR(DEFAULT_COLORS));
    }
    // D: place defence-tower
    else if (choice == 'D') {
      str = player_->add_def();
      if (str != "")
        attron(COLOR_PAIR(ERROR));
      else
        field_->get_new_defence_pos(true);
      str.insert(str.length(), field_->cols()*2-str.length(), char(46));
      std::unique_lock ul(mutex_print_field_);
      mvaddstr(LINE_MSG, 10, str.c_str());
      attron(COLOR_PAIR(DEFAULT_COLORS));
    }
    print_field();
    refresh();
  } 
}

void Game::print_field() {
  std::unique_lock ul(mutex_print_field_);
  mvaddstr(LINE_HELP, 10, HELP);
  field_->print_field(player_, ki_);
  mvaddstr(LINE_STATUS, 10, player_->get_status().c_str());
}
