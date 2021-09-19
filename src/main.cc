#include <curses.h>
#include <filesystem>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sstream>
#include "audio/audio.h"
#include "game/game.h"

#include <spdlog/spdlog.h>
#include "spdlog/common.h"
#include "spdlog/sinks/basic_file_sink.h"

#define LOGGER "logger"


#define ITERMAX 10000

int main(void) {

  auto logger = spdlog::basic_logger_mt("logger", "logs/basic-log.txt");
  spdlog::flush_every(std::chrono::seconds(1));
  spdlog::flush_on(spdlog::level::debug);
  spdlog::set_level(spdlog::level::debug);
  // std::stringstream buffer;
  // std::streambuf * old = std::cout.rdbuf(buffer.rdbuf());
  // std::cout << old << std::endl;

  Audio::CreateKeys();
  /*
  for (const auto& key : Audio::keys()) {
    std::cout << key.first << ": ";
    for (const auto& note : key.second) 
      std::cout << note << ", ";
    std::cout << std::endl;
  }
  Audio audio;

  for (const auto& path : std::filesystem::directory_iterator("data/songs/")) {
    audio.set_source_path(path.path());
    audio.Analyze();
    // spdlog::get(LOGGER)->info("{}: Tactics: ", path.path().filename().string());
    // AudioKi ki = AudioKi({1, 1}, 1, audio.analysed_data().average_bpm_, audio.analysed_data().average_level_, NULL);
    // ki.SetUpTactics(audio.analysed_data().intervals_.begin()->second);

    // for (const auto& it : audio.analysed_data().intervals_) 
    //   std::cout << "key: " << it.second.key_ << ", darkness: " << it.second.darkness_ << std::endl;
    // std::cout << std::endl;
  }

  return 0;
  */

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
