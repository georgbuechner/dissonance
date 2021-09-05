#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "game.h"

#include <spdlog/spdlog.h>
#include "spdlog/sinks/basic_file_sink.h"

#define LOGGER "logger"


#define ITERMAX 10000

int main(void) {

  auto logger = spdlog::basic_logger_mt("logger", "logs/basic-log.txt");
  spdlog::flush_every(std::chrono::seconds(1));
  spdlog::flush_on(spdlog::level::err);
  spdlog::set_level(spdlog::level::debug);

  srand (time(NULL));

  // initialize curses
  setlocale(LC_ALL, "");
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, true);

  clear();

  // init colors
  use_default_colors();
  start_color();
  init_pair(1, COLOR_BLUE, -1);
  init_pair(2, COLOR_RED, -1);
  init_pair(3, -1, -1);
  init_pair(4, COLOR_CYAN, -1);
  init_pair(5, COLOR_GREEN, -1);
  init_pair(6, COLOR_MAGENTA, -1);

  // initialize game.
  Game game((LINES-20), (COLS-40) / 2);

  // start game
  game.play();
  
  refresh();

  endwin();

  exit(0);
}
