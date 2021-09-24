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

#define LOGGER "logger"


#define ITERMAX 10000

int main(int argc, const char** argv) {

  // Logger 
  auto logger = spdlog::basic_logger_mt("logger", "logs/basic-log.txt");
  spdlog::flush_every(std::chrono::seconds(1));
  spdlog::flush_on(spdlog::level::debug);
  spdlog::set_level(spdlog::level::debug);

  // Command line arguments 
  bool relative_size = false;
  bool show_help = false;
  std::string audio_base_path = getenv("HOME");
  audio_base_path += "/.disonance/data";

  auto cli = lyra::cli() 
    | lyra::opt(relative_size) ["-r"]["--relative-size"]("If set, adjusts map size to terminal size.")
    | lyra::opt(audio_base_path, "path to audio files") ["-p"]["--audio-base-path"]("Set path to audio data");
    
  cli.add_argument(lyra::help(show_help));
  auto result = cli.parse({ argc, argv });

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

  // Initialize audio
  Audio::Initialize();

  // Initialize random numbers.
  srand (time(NULL));

  // Initialize curses
  setlocale(LC_ALL, "");
  initscr();
  cbreak();
  noecho();
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
  int lines = 52;
  int cols  = 74;
  int left_border = (COLS - cols) /2 - 40;
  if (relative_size) {
    lines = LINES-20;
    cols = (COLS-40)/2;
    left_border = 10;
  }
  // Initialize game.
  Game game(lines, cols, left_border, audio_base_path);
  // Start game
  game.play();
  
  // Wrap up.
  refresh();
  clear();
  endwin();
  exit(0);
}
