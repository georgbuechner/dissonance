#include <chrono>
#include <cstdlib>
#include <curses.h>
#include <filesystem>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sstream>
#include <stdlib.h>
#include <lyra/lyra.hpp>
#include "audio/audio.h"
#include "game/game.h"

#include <spdlog/spdlog.h>
#include "lyra/help.hpp"
#include "spdlog/common.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "utils/utils.h"

#define LOGGER "logger"
#define ITERMAX 10000

int main(int argc, const char** argv) {
  // Command line arguments 
  bool relative_size = false;
  bool show_help = false;
  bool clear_log = false;
  std::string log_level = "warn";
  std::string base_path = getenv("HOME");
  base_path += "/.dissonance/";

  auto cli = lyra::cli() 
    | lyra::opt(relative_size) ["-r"]["--relative-size"]("If set, adjusts map size to terminal size.")
    | lyra::opt(clear_log) ["-c"]["--clear-log"]("If set, removes all log-files before starting the game.")
    | lyra::opt(log_level, "options: [warn, info, debug], default: \"warn\"") ["-l"]["--log_level"]("set log-level")
    | lyra::opt(base_path, "path to dissonance files") ["-p"]["--base-path"]("Set path to dissonance files (logs, settings, data)");
    
  cli.add_argument(lyra::help(show_help));
  auto result = cli.parse({ argc, argv });

  // help
  if (show_help) {
    std::cout << cli;
    return 0;
  }
  // clear log
  if (clear_log)
    std::filesystem::remove_all(base_path + "logs/");
  // Create map based on actual terminal size.
  if (relative_size)
    relative_size = true;

  // Logger 
  std::string logger_file = "logs/" + utils::GetFormatedDatetime() + "_logfile.txt";
  auto logger = spdlog::basic_logger_mt("logger", base_path + logger_file);
  spdlog::flush_on(spdlog::level::warn);
  if (log_level == "warn")
    spdlog::set_level(spdlog::level::warn);
  else if (log_level == "info")
    spdlog::set_level(spdlog::level::info);
  else if (log_level == "debug") {
    spdlog::set_level(spdlog::level::debug);
    spdlog::flush_on(spdlog::level::info);
    spdlog::flush_every(std::chrono::seconds(1));
  }

  // Initialize audio
  Audio::Initialize();

  // Initialize random numbers.
  srand (time(NULL));

  // Initialize curses
  setlocale(LC_ALL, "");
  initscr();
  cbreak();
  noecho();
  curs_set(0);
  keypad(stdscr, true);
  clear();
  
  // Initialize colors.
  use_default_colors();
  start_color();
  init_pair(COLOR_AVAILIBLE, COLOR_BLUE, -1);
  init_pair(COLOR_ERROR, COLOR_RED, -1);
  init_pair(COLOR_DEFAULT, -1, -1);
  init_pair(COLOR_MSG, COLOR_CYAN, -1);
  init_pair(COLOR_SUCCESS, COLOR_GREEN, -1);
  init_pair(COLOR_MARKED, COLOR_MAGENTA, -1);

  // Setup map-size
  int lines = 40;
  int cols  = 74;
  int left_border = (COLS - cols) /2 - 40;
  if (relative_size) {
    lines = LINES-20;
    cols = (COLS-40)/2;
    left_border = 10;
  }
  // Initialize game.
  Game game(lines, cols, left_border, base_path);
  // Start game
  game.play();
  
  // Wrap up.
  refresh();
  clear();
  endwin();
  exit(0);
}
