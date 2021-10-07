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

void UpdateTerminfo() {
  std::array<char, 128> buffer;
  std::string result = "";
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen("whereis terminfo | awk '{print $2}'", "r"), pclose);
  if (!pipe)
    throw std::runtime_error("popen() failed!");
  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
    result += buffer.data();
  setenv("TERMINFO", result.c_str(), 2);
}

int main(int argc, const char** argv) {
  // Command line arguments 
  bool relative_size = false;
  bool dont_update_terminfo = false;
  bool show_help = false;
  std::string base_path = getenv("HOME");
  base_path += "/.dissonance/";

  auto cli = lyra::cli() 
    | lyra::opt(relative_size) ["-r"]["--relative-size"]("If set, adjusts map size to terminal size.")
    | lyra::opt(dont_update_terminfo) ["-i"]["--no-terminfo"]("If set, terminfo is not updated.")
    | lyra::opt(base_path, "path to dissonance files") ["-p"]["--base-path"]("Set path to dissonance files (logs, settings, data)");
    
  cli.add_argument(lyra::help(show_help));
  auto result = cli.parse({ argc, argv });
  
  // Logger 
  auto logger = spdlog::basic_logger_mt("logger", base_path + "logs/basic-log.txt");
  spdlog::flush_every(std::chrono::seconds(1));
  spdlog::flush_on(spdlog::level::debug);
  spdlog::set_level(spdlog::level::debug);

  if (show_help) {
      std::cout << cli;
      return 0;
  }
  // The parser with the one option argument:
  if (relative_size) {
    relative_size = true;
    spdlog::get(LOGGER)->info("using relative size.");
  }
  else 
    spdlog::get(LOGGER)->info("using fixed size.");
  // Update terminfo
  if (!dont_update_terminfo) {
    UpdateTerminfo();
    spdlog::get(LOGGER)->debug("Updated terminfo");
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
