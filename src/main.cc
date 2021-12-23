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

int main(int argc, const char** argv) {
  // Command line arguments 
  bool relative_size = false;
  bool show_help = false;
  bool clear_log = false;
  std::string log_level = "warn";
  std::string base_path = getenv("HOME");
  base_path += "/.dissonance/";
  
  // Initialize random numbers.
  srand (time(NULL));

  int server_port = 4444;
  bool muli_player = false;
  std::string server_address = "ws://localhost:4444";
  bool standalone = false;

  auto cli = lyra::cli() 
    | lyra::opt(relative_size) ["-r"]["--relative-size"]("If set, adjusts map size to terminal size.")
    | lyra::opt(clear_log) ["-c"]["--clear-log"]("If set, removes all log-files before starting the game.")
    | lyra::opt(muli_player) ["-m"]["--muli-player"]("If set, starts a multi-player game.")
    | lyra::opt(standalone) ["-s"]["--standalone"]("If set, starts only server.")
    | lyra::opt(server_address, "format [ws://<url>:<port> | wss://<url>:<port>], default: wss://kava-i.de:4444") 
        ["-z"]["--connect"]("specify address which to connect to.")
    | lyra::opt(log_level, "options: [warn, info, debug], default: \"warn\"") ["-l"]["--log_level"]("set log-level")
    | lyra::opt(base_path, "path to dissonance files") 
        ["-p"]["--base-path"]("Set path to dissonance files (logs, settings, data)");
    
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
  else if (log_level == "info") {
    spdlog::set_level(spdlog::level::info);
    spdlog::flush_on(spdlog::level::info);
  }
  else if (log_level == "debug") {
    spdlog::set_level(spdlog::level::debug);
    spdlog::flush_on(spdlog::level::debug);
    spdlog::flush_every(std::chrono::seconds(1));
  }

  std::string username = "";
  if (!standalone) {
    std::cout << "Enter your username: ";
    std::getline(std::cin, username);
  }

  std::cout << "Base path: " << base_path << std::endl;
  
  // Initialize audio
  Audio::Initialize();
  
  // Create websocket server.
  WebsocketServer* srv = new WebsocketServer(standalone);
  std::thread thread_server([srv, server_port, muli_player, standalone]() { 
    if (!muli_player) {
      if (standalone)
        std::cout << "Server started on port: " << server_port << std::endl;
      srv->Start(server_port);
    }
  });

  // Create client and client-game.
  ClientGame* client_game = (standalone) ? nullptr : new ClientGame(relative_size, base_path, username, muli_player);
  Client* client = (standalone) ? nullptr : new Client(client_game, username);
  if (client_game)
    client_game->set_client(client);
  std::thread thread_client([client, server_address]() { if (client) client->Start(server_address); });
  std::thread thread_client_input([client_game, client]() { 
    if (client) {
      client_game->GetAction(); 
      sleep(1);
      client->Stop();
    }
  });

  thread_server.join();
  thread_client.join();
  thread_client_input.join();
}
