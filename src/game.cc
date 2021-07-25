#include "game.h"
#include <bits/types/time_t.h>
#include <cstdlib>
#include <curses.h>
#include <time.h>
#include <thread>

#define HELP "soldier [a], gatherer [r], tech [t] | barracks [A], resources [R], tech [t] | pause [space] quit [q]"
#define DEFAULT_COLORS 3
#define ERROR 2 
#define MSG 4

#define LINE_HELP 8
#define LINE_STATUS LINES-9
#define LINE_MSG LINES-8

Game::Game(int lines, int cols) {
  field_ = new Field(lines, cols);
  field_->add_hills();
  auto player_den_pos = field_->add_player();
  auto ki_den_pos = field_->add_ki();

  player_ = new Player(player_den_pos);
  ki_ = new Player(ki_den_pos);
}

void Game::play() {
  std::thread thread_actions([this]() { do_actions(); });
  std::thread thread_choices([this]() { (get_player_choice()); });
  thread_actions.join();
  thread_choices.join();
}

void Game::do_actions() {
  time_t last_action = time(NULL);
  while (true) {
    time_t cur_time = time(NULL);
    if (cur_time - last_action > 1) {
      last_action = cur_time;
      player_->inc();
      player_->update_soldiers();
      refresh();
      print_field();
    }
  } 
}

void Game::get_player_choice() {
  int c;
  std::string str = "";
  print_field();
  while (true) {
    c = getch();
    if (c == 'q') {
      refresh();
      endwin();
      exit(0);
    }
    else if (c == 'r') {
      attron(COLOR_PAIR(MSG));
      mvaddstr(LINE_MSG, 10, "gold-, bronze- or silver-gatherer ([g]/[b]/[s])");
      attron(COLOR_PAIR(DEFAULT_COLORS));
      c = getch();
      if (c == 'b')
        str = player_->add_gatherer_bronze();
      else if (c == 's')
        str = player_->add_gatherer_silver();
      else if (c == 'g')
        str = player_->add_gatherer_gold();
      // Use error color in case of error.
      if (str != "")
        attron(COLOR_PAIR(ERROR));
      str.insert(str.length(), field_->cols()*2-str.length(), char(46));
      mvaddstr(LINE_MSG, 10, str.c_str());
      attron(COLOR_PAIR(DEFAULT_COLORS));
    }
    else if (c == 'a') {
      auto pos = field_->get_new_soldier_pos();
      str = player_->add_soldier(pos);
      if (str != "")
        attron(COLOR_PAIR(ERROR));
      str.insert(str.length(), field_->cols()*2-str.length(), char(46));
      mvaddstr(LINE_MSG, 10, str.c_str());
      attron(COLOR_PAIR(DEFAULT_COLORS));
       
    }
    refresh();
    print_field();
  } 
}

void Game::print_field() {
  mvaddstr(LINE_HELP, 10, HELP);
  field_->print_field(player_, ki_);
  mvaddstr(LINE_STATUS, 10, player_->get_status().c_str());
}


