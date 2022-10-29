/*
 * @author: fux
 */
#include <filesystem>
#include <thread>
#include <vector>
#include <spdlog/spdlog.h>
#include "share/constants/texts.h"
#include "server/game/player/player.h"
#include "server/websocket/websocket_server.h"
#include "share/tools/utils/utils.h"
#include "share/constants/codes.h"

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

//Function only needed for callback, but nether actually used.
inline std::string get_password() { return ""; }

WebsocketServer::WebsocketServer(bool standalone, std::string base_path) 
  : standalone_(standalone), base_path_(base_path) {
  lobby_ = std::make_shared<Lobby>();
}

WebsocketServer::~WebsocketServer() {
  server_.stop();
}

void WebsocketServer::Start(int port) {
  try { 
    spdlog::get(LOGGER)->info("WebsocketFrame::Start: set_access_channels");
    server_.set_access_channels(websocketpp::log::alevel::none);
    server_.clear_access_channels(websocketpp::log::alevel::frame_payload);
    server_.init_asio(); 
    server_.set_message_handler(bind(&WebsocketServer::on_message, this, &server_, ::_1, ::_2)); 
    server_.set_open_handler(bind(&WebsocketServer::OnOpen, this, ::_1));
    server_.set_close_handler(bind(&WebsocketServer::OnClose, this, ::_1));
    server_.set_reuse_addr(true);
    server_.listen(port); 
    server_.start_accept(); 
    spdlog::get(LOGGER)->info("WebsocketFrame:: successfully started websocket server on port: {}", port);
    server_.run(); 
  } catch (websocketpp::exception const& e) {
    std::cout << "WebsocketFrame::Start: websocketpp::exception: " << e.what() << std::endl;
  } catch (std::exception& e) {
    std::cout << "WebsocketFrame::Start: websocketpp::exception: " << e.what() << std::endl;
  }
}

void WebsocketServer::OnOpen(websocketpp::connection_hdl hdl) {
  std::unique_lock ul_connections(mutex_connections_);
  if (connections_.count(hdl.lock().get()) == 0)
    connections_[hdl.lock().get()] = new Connection(hdl, hdl.lock().get(), "");
  else
    spdlog::get(LOGGER)->warn("WebsocketFrame::OnOpen: New connection, but connection already exists!");
}

void WebsocketServer::OnClose(websocketpp::connection_hdl hdl) {
  std::unique_lock ul_connections(mutex_connections_);
  if (connections_.count(hdl.lock().get()) > 0) {
    try {
      // Delete connection.
      std::string username = connections_.at(hdl.lock().get())->username();
      delete connections_[hdl.lock().get()];
      if (connections_.size() > 1)
        connections_.erase(hdl.lock().get());
      else 
        connections_.clear();
      ul_connections.unlock();
      spdlog::get(LOGGER)->info("WebsocketFrame::OnClose: Connection closed successfully: {}", username);
      // Get and lock game.
      {
        auto [game, game_id] = GetGameFromUsername(username);
        // Tell game that player has disconnected. 
        if (game)
          game->PlayerResigned(username);
        UnlockGame(game_id);
      }
      // If player was host (game exists with this username): update lobby for waiting players
      std::shared_lock sl_games(mutex_games_map_);
      if (games_.count(username) > 0) {
        spdlog::get(LOGGER)->debug("WebsocketFrame::OnClose: Game removed, informing waiting players");
        SendUpdatedLobbyToAllInLobby();
      }
    } catch (std::exception& e) {
      spdlog::get(LOGGER)->error("WebsocketFrame::OnClose: Failed to close connection: {}", e.what());
    }
  }
  else 
    spdlog::get(LOGGER)->info("WebsocketFrame::OnClose: Connection closed, but connection didn't exist!");
}

void WebsocketServer::on_message(server* srv, websocketpp::connection_hdl hdl, message_ptr msg) {
  // try parsing dto:
  Command cmd;
  try {
    cmd = Command(msg->get_payload().c_str(), msg->get_payload().size());
    spdlog::get(LOGGER)->debug("Websocket::on_message: new message with command: {}", cmd.command());
  }
  catch (std::exception& e) {
    spdlog::get(LOGGER)->warn("Websocket::on_message: failed parsing message: {}", e.what());
    return;
  }

  // Create controller connection
  if (cmd.command() == "initialize_user")
    h_InitializeUser(hdl.lock().get(), cmd.username());
  // Start new game
  else if (cmd.command() == "setup_new_game")
    h_InitializeGame(hdl.lock().get(), cmd.username(), cmd.data());
  // Indicate that player is ready (audio downloaded).
  else if (cmd.command() == "ready") {
    if (username_game_id_mapping_.count(cmd.username()) > 0)
      games_.at(username_game_id_mapping_.at(cmd.username()))->PlayerReady(cmd.username());
  }
  // In game action
  else {
    std::thread handler([this, hdl, cmd]() { h_InGameAction(hdl.lock().get(), cmd); });
    handler.detach();
  }
  spdlog::get(LOGGER)->debug("Websocket::on_message: exited");
}

void WebsocketServer::h_InitializeUser(connection_id id, std::string username) {
  // Create controller connection.
  // Username already exists.
  if (connections_.count(id) > 0 && UsernameExists(username))
    SendMessage(id, "kill", std::make_shared<Msg>("Username exists!"));
  // Game exists with current username
  else if (username_game_id_mapping_.count(username))
    SendMessage(id, "kill", std::make_shared<Msg>("A game for this username is currently running!"));
  // Everything fine: set username and send next instruction to user.
  else if (connections_.count(id) > 0) {
    std::shared_lock sl_connections(mutex_connections_);
    connections_[id]->set_username(username);
    sl_connections.unlock();
    SendMessage(id, "select_mode", std::make_shared<Paragraph>(texts::server_msg));
  }
  else
    spdlog::get(LOGGER)->warn("WebsocketServer::h_InitializeUser: connection does not exist! {}", username);
}

void WebsocketServer::h_InitializeGame(connection_id id, std::string username, std::shared_ptr<Data> data) {
  if (data->mode() == SINGLE_PLAYER || data->mode() == TUTORIAL) {
    spdlog::get(LOGGER)->info("WebsocketServer::h_InitializeGame: initializing new single-player game.");
    std::string game_id = username;
    std::unique_lock ul_games_map(mutex_games_map_); // unique_lock since games-map is being modyfied (add game)
    username_game_id_mapping_[username] = game_id;
    games_[game_id] = std::make_shared<ServerGame>(data->lines(), data->cols(), data->mode(), 2, base_path_, this);
    SendMessage(id, Command("select_audio").bytes());
  }
  else if (data->mode() == MULTI_PLAYER) {
    spdlog::get(LOGGER)->info("WebsocketServer::h_InitializeGame: initializing new multi-player game.");
    std::string game_id = username;
    std::unique_lock ul_games_map(mutex_games_map_); // unique_lock since games-map is being modyfied (add game)
    username_game_id_mapping_[username] = game_id;
    games_[game_id] = std::make_shared<ServerGame>(data->lines(), data->cols(), data->mode(), data->num_players(), 
        base_path_, this);
    SendMessage(id, Command("select_audio").bytes());
    spdlog::get(LOGGER)->debug("WebsocketServer::h_InitializeGame: New game added, informing waiting players");
  }
  else if (data->mode() == MULTI_PLAYER_CLIENT) {
    spdlog::get(LOGGER)->info("WebsocketServer::h_InitializeGame: initializing new client-player.");
    std::string game_id = data->game_id();
    // game_id empy: enter lobby
    if (game_id == "") {
      spdlog::get(LOGGER)->debug("WebsocketServer::h_InitializeGame: new client-player: get lobby");
      UpdateLobby();
      SendMessage(username, "update_lobby", lobby_);
      std::shared_lock sl_connections(mutex_connections_);
      connections_.at(id)->set_waiting(true); // indicate player is now waiting for game.
      sl_connections.unlock();
    }
    // with game_id: joint game
    else if (games_.count(game_id) > 0) {
      spdlog::get(LOGGER)->debug("WebsocketServer::h_InitializeGame: add client-player");
      std::unique_lock ul_games_map(mutex_games_map_);  // unique-lock since user-game mapping is modyfied
      LockGame(game_id); 
      auto game = games_.at(game_id);
      if (game->status() == WAITING_FOR_PLAYERS) {
        std::shared_lock sl_connections(mutex_connections_);
        connections_.at(id)->set_waiting(false);  // indicate player has stopped waiting for game.
        sl_connections.unlock();
        username_game_id_mapping_[username] = game_id;  // Add username to mapping, to find matching game.
        game->AddPlayer(username, data->lines(), data->cols()); // Add user to game.
        UnlockGame(game_id); 
        SendMessage(id, "print_msg", std::make_shared<Msg>("Waiting for players..."));
      }
      else{ 
        spdlog::get(LOGGER)->debug("WebsocketServer::h_InitializeGame: add client-player: game full");
        SendMessage(id, "print_msg", std::make_shared<Msg>("Game already full."));
      }
    }
    else {
      spdlog::get(LOGGER)->info("WebsocketServer::h_InitializeGame: add client-player: game does not exist.");
      SendMessage(id, "print_msg", std::make_shared<Msg>("Game no longer exists"));
    }
  }
  else if (data->mode() == OBSERVER) {
    spdlog::get(LOGGER)->info("Server: initializing new observer game.");
    std::string game_id = username;
    std::unique_lock ul(mutex_games_map_);
    username_game_id_mapping_[username] = game_id;
    games_[game_id] = std::make_shared<ServerGame>(data->lines(), data->cols(), data->mode(), 2, base_path_, this);
    SendMessage(id, Command("select_audio").bytes());
  }
}

void WebsocketServer::h_InGameAction(connection_id id, Command cmd) {
  spdlog::get(LOGGER)->debug("h_InGameAction: username {}", cmd.username());
  // Gets command and adds command to cmd-data.
  std::string command = cmd.command();
  cmd.data()->AddUsername(cmd.username());
  // Get and lock game.
  auto [game, game_id] = GetGameFromUsername(cmd.username());
  if (game) {
    try {
      game->HandleInput(command, cmd.data());
    } catch (std::exception& e) {
      spdlog::get(LOGGER)->error("h_InGameAction: calling game-function failed: {}", e.what());
    }
    UnlockGame(game_id);
    // Update lobby for all waiting players after "initialize_game" was called (potentially new game)
    if (command == "initialize_game")
      SendUpdatedLobbyToAllInLobby();
  }
  else
    spdlog::get(LOGGER)->warn("Server: message with unkown command ({}) or username ({})", cmd.username(), cmd.command());
  spdlog::get(LOGGER)->debug("h_InGameAction: for done for username {}.", cmd.username());
}

void WebsocketServer::CloseGames() {
  for(;;) {
    spdlog::get(LOGGER)->debug("WebsocketServer::CloseGames: waiting...");
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    std::unique_lock ul_games_map(mutex_games_map_); // unique_lock since 
    std::vector<std::string> games_to_delete;
    spdlog::get(LOGGER)->debug("WebsocketServer::CloseGames: Checking running games");
    // Gather games to delete or close games if all users are disconnected.
    for (const auto& it : games_) {
      // If game is closed add to game_to_delete.
      if (it.second->status() == CLOSED)
        games_to_delete.push_back(it.first);
      // If all users are disconnected, set status to closed.
      else if (GetPlayingUsers(it.first, true).size() == 0)
        it.second->set_status((it.second->status() <= SETTING_UP) ? CLOSED : CLOSING);
    }
    // Detelte games to delete, remove from games and remove all player-game-mappings for this game.
    for (const auto& it : games_to_delete) {
      // Check if game is locked, if yes, continue...
      if (IsLocked(it))
        continue;
      // Delete game by removeing from list of games.
      games_.erase(it);
      spdlog::get(LOGGER)->info("WebsocketServer::CloseGames: removed game with id: {}", it);
      // Remove all username-game-mappings
      std::vector<std::string> all_users = GetPlayingUsers(it);
      for (const auto& it : all_users)
        username_game_id_mapping_.erase(it);
      // Exit if not standalone-server.
      if (!standalone_) {
        spdlog::get(LOGGER)->info("Shutting down server");
        return;
      }
    }
    ul_games_map.unlock();
    if (games_to_delete.size() > 0) {
      spdlog::get(LOGGER)->debug("WebsocketServer::CloseGames: Game removed, informing waiting players");
      SendUpdatedLobbyToAllInLobby();
    }
  }
}

WebsocketServer::connection_id WebsocketServer::GetConnectionIdByUsername(std::string username) {
  std::shared_lock sl_connections(mutex_connections_);
  for (const auto& it : connections_)
    if (it.second->username() == username) 
      return it.second->get_connection_id();
  return connection_id();
}

bool WebsocketServer::UsernameExists(std::string username) {
  std::shared_lock sl_connections(mutex_connections_);
  for (const auto& it : connections_)
    if (it.second->username() == username) 
      return true;
  return false;
}

std::pair<std::shared_ptr<ServerGame>, std::string> WebsocketServer::GetGameFromUsername(std::string username) {
  std::shared_lock sl_games_map(mutex_games_map_);
  std::shared_lock sl_connections(mutex_connections_);
  if (username_game_id_mapping_.count(username) > 0 && games_.count(username_game_id_mapping_.at(username))) {
    std::string game_id = username_game_id_mapping_.at(username);
    LockGame(game_id);
    return {games_.at(game_id), game_id};
  }
  return {nullptr, ""};
}

std::vector<std::string> WebsocketServer::GetPlayingUsers(std::string username, bool check_connected) {
  std::vector<std::string> all_playering_users;
  if (username_game_id_mapping_.count(username) > 0) {
    std::string game_id = username_game_id_mapping_.at(username);
    for (const auto& it : username_game_id_mapping_) {
      if (it.second == game_id && (UsernameExists(username) || !check_connected))
        all_playering_users.push_back(it.first);
    }
  }
  return all_playering_users;
}

void WebsocketServer::SendMessage(std::string username, Command& cmd) {
  spdlog::get(LOGGER)->debug("WebsocketFrame::SendMessage: username: {}, command: {}", cmd.username(), cmd.command());
  SendMessage(GetConnectionIdByUsername(username), cmd.bytes());
}

void WebsocketServer::SendMessage(std::string username, std::string command, std::shared_ptr<Data> data) {
  spdlog::get(LOGGER)->debug("WebsocketFrame::SendMessage: username: {}, command: {}", username, command);
  SendMessage(GetConnectionIdByUsername(username),  Command(command, data).bytes());
}

void WebsocketServer::SendMessage(connection_id id, std::string command, std::shared_ptr<Data> data) {
  spdlog::get(LOGGER)->debug("WebsocketFrame::SendMessage: id: {}, command: {}", id, command);
  Command cmd(Command(command, data));
  SendMessage(id, cmd.bytes());
}

void WebsocketServer::SendMessage(connection_id id, std::string msg) {
  try {
    // Set last incomming and get connection hdl from connection.
    std::shared_lock sl_connections(mutex_connections_);
    if (connections_.count(id) == 0) {
      spdlog::get(LOGGER)->error("WebsocketFrame::SendMessage: failed to get connection to {}", id);
      return;
    }
    // Send message.
    server_.send(connections_.at(id)->connection(), msg, websocketpp::frame::opcode::binary);
    spdlog::get(LOGGER)->debug("WebsocketFrame::SendMessage: successfully sent message.");
  } catch (websocketpp::exception& e) {
    spdlog::get(LOGGER)->error("WebsocketFrame::SendMessage: failed sending message: {}", e.what());
  } catch (std::exception& e) {
    spdlog::get(LOGGER)->error("WebsocketFrame::SendMessage: failed sending message: {}", e.what());
  }
}

void WebsocketServer::SendUpdatedLobbyToAllInLobby() {
  UpdateLobby();
  for (const auto& it : connections_) {
    if (it.second->waiting())
      SendMessage(it.second->username(), "update_lobby", lobby_);
  }
}

void WebsocketServer::UpdateLobby() {
  std::unique_lock ul_lobby(mutex_lobby_);
  std::shared_lock sl_games_map(mutex_games_map_);
  lobby_->clear();
  for (const auto& it : games_) {
    if (it.second->status() == WAITING_FOR_PLAYERS)
      lobby_->AddEntry(it.first, it.second->max_players(), it.second->cur_players(), it.second->audio_map_name());
  }
}

void WebsocketServer::LockGame(std::string game_id) {
  std::unique_lock ul_games_lock(mutex_games_lock_);
  games_lock_[game_id] = true;
}

void WebsocketServer::UnlockGame(std::string game_id) {
  std::unique_lock ul_games_lock(mutex_games_lock_);
  if (games_lock_.count(game_id) > 0)
    games_lock_[game_id] = false;
  else 
    spdlog::get(LOGGER)->warn("WebsocketServer::UnlockGame: trying to unlock game, but {} does not exist!", game_id);
}

bool WebsocketServer::IsLocked(std::string game_id) {
  std::unique_lock ul_games_lock(mutex_games_lock_);
  if (games_lock_.count(game_id) > 0)
    return games_lock_.at(game_id);
  return false;
}
