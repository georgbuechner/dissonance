#include "nlohmann/json_fwd.hpp"
#include "server/game/server_game.h"
#include "share/constants/codes.h"
#define NCURSES_NOMACROS
#include <chrono>
#include <cstdlib>
#include <curses.h>
#include <filesystem>
#include <lyra/lyra.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sstream>
#include <stdlib.h>
#include <spdlog/spdlog.h>
#include "spdlog/common.h"
#include "spdlog/sinks/basic_file_sink.h"

#include "share/audio/audio.h"
#include "server/websocket/websocket_server.h"
#include "client/websocket/client.h"
#include "client/game/client_game.h"
#include "share/tools/utils/utils.h"

#define LOGGER "logger"
#define ITERMAX 10000

/**
 * Sets up logger and potentially clears old logs.
 * @param[in] clear_log (if set, empties log-folder at base-path)
 * @param[in] base_path
 * @param[in] log_level.
 */
void SetupLogger(bool clear_log, std::string base_path, std::string log_level);

int main(int argc, const char** argv) {
  std::cout << "Starting version 2.0.1" << std::endl;
  // Command line arguments 
  bool show_help = false;
  bool keep_log = false;
  std::string log_level = "error";
  std::string base_path = getenv("HOME");
  base_path += "/.dissonance/";
  int server_port = 4444;
  bool multiplayer = false;
  std::string server_address = "ws://localhost:4444";
  bool standalone = false;
  std::string path_sound_map = "dissonance/data/examples/Hear_My_Call-coffeeshoppers.mp3";
  std::string path_sound_ai_1 = "dissonance/data/examples/airtone_-_blackSnow_1.mp3";
  std::string path_sound_ai_2 = "dissonance/data/examples/Karstenholymoly_-_The_night_is_calling.mp3";

  // Setup command-line-arguments-parser
  auto cli = lyra::cli() 
    // Standard settings (clear-log, log-level and base-path)
    | lyra::opt(keep_log) ["-k"]["--keep-log"]("If set, does not remove all log-files before starting the game.")
    | lyra::opt(log_level, "options: [warn, info, debug], default: \"warn\"") ["-l"]["--log_level"]("set log-level")
    | lyra::opt(base_path, "path to dissonance files") 
        ["-p"]["--base-path"]("Set path to dissonance files (logs, settings, data)")

    // multi-player/ standalone.
    | lyra::opt(multiplayer) ["-m"]["--multiplayer"]("If set, starts a multi-player game.")
    | lyra::opt(standalone) ["-s"]["--standalone"]("If set, starts only server.")
    | lyra::opt(server_address, "format [ws://<url>:<port> | wss://<url>:<port>], default: wss://kava-i.de:4444") 
        ["-z"]["--connect"]("specify address which to connect to.")

    | lyra::opt(path_sound_map, "for ai games: map sound input") ["--map_sound"]("")
    | lyra::opt(path_sound_ai_1, "for ai games: ai-1 sound input") ["--ai1_sound"]("")
    | lyra::opt(path_sound_ai_2, "for ai games: ai-2 sound input") ["--ai2_sound"]("");

  cli.add_argument(lyra::help(show_help));
  auto result = cli.parse({ argc, argv });

  // Print help and exit. 
  if (show_help) {
    std::cout << cli;
    return 0;
  }

  // Setup logger.
  SetupLogger(keep_log, base_path, log_level);
  
  // Initialize random numbers and audio.
  srand (time(NULL));
  Audio::Initialize();

  // Enter-username (omitted for standalone server or only-ai)
  std::string username = "";
  if (!standalone) {
    std::cout << "Using base-path: " << base_path << std::endl;
    std::cout << "Enter your username: ";
    std::getline(std::cin, username);
  }

  /* 
   * Start games: 
   * 1. websocket-server (for single-player and standalone)
   * 2. client (for multi-player)
   *
   * 1. and 2. depend on `--standalone` and `--multiplayer`. 
   * If a) both are set, start websocket-server (localhost) and client.
   * If b) only `--standalone` is set, start only server at given adress.
   * If c) only `--multiplayer` is set, start only client.
  */

  // websocket server.
  WebsocketServer* srv = new WebsocketServer(standalone, base_path);
  std::thread thread_server([srv, server_port, multiplayer, standalone]() { 
    if (!multiplayer) {
      if (standalone)
        std::cout << "Server started on port: " << server_port << std::endl;
      srv->Start(server_port);
    }
  });
  std::thread thread_kill_games([srv, multiplayer]() {
    if (!multiplayer)
      srv->CloseGames();
  });

  // client and client-game (only if not standalone)
  ClientGame::init();
  ClientGame* client_game = (standalone) ? nullptr : new ClientGame(base_path, username, multiplayer);
  Client* client = (standalone) ? nullptr : new Client(client_game, username, base_path);
  if (client_game)
    client_game->set_client(client);
  std::thread thread_client([client, server_address]() { if (client) client->Start(server_address); });
  std::thread thread_client_input([client_game, client]() { 
    if (client) {
      client_game->GetAction(); 
      sleep(1);
    }
  });

  thread_server.join();
  thread_kill_games.join();
  thread_client.join();
  thread_client_input.join();
}

void SetupLogger(bool keep_log, std::string base_path, std::string log_level) {
  // clear log
  if (!keep_log)
    std::filesystem::remove_all(base_path + "logs/");

  // Logger 
  std::string logger_file = "logs/" + utils::GetFormatedDatetime() + "_logfile.txt";
  auto logger = spdlog::basic_logger_mt("logger", base_path + logger_file);
  spdlog::flush_on(spdlog::level::warn);
  if (log_level == "error")
    spdlog::set_level(spdlog::level::err);
  else if (log_level == "warn")
    spdlog::set_level(spdlog::level::warn);
  else if (log_level == "info") {
    spdlog::set_level(spdlog::level::info);
    spdlog::flush_on(spdlog::level::info);
  }
  else if (log_level == "debug") {
    spdlog::set_level(spdlog::level::debug);
    spdlog::flush_on(spdlog::level::debug);
    spdlog::flush_every(std::chrono::seconds(1));
  }
}
