/*
 * @author: georgbuechner
 */
#include <exception>
#include <filesystem>
#include <ostream>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/types.h>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>
#include <websocketpp/common/connection_hdl.hpp>
#include <websocketpp/error.hpp>
#include <websocketpp/frame.hpp>
#include <spdlog/spdlog.h>
#include "nlohmann/json_fwd.hpp"
#include "spdlog/common.h"
#include "spdlog/logger.h"
#include "websocketpp/close.hpp"
#include "websocketpp/common/system_error.hpp"
#include "websocketpp/logger/levels.hpp"

#include "server/websocket/websocket_server.h"
#include "share/tools/utils/utils.h"
#include "share/constants/codes.h"

#ifdef _COMPILE_FOR_SERVER_
namespace asio = websocketpp::lib::asio;
#endif

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

//Function only needed for callback, but nether actually used.
inline std::string get_password() { return ""; }

WebsocketServer::WebsocketServer(bool standalone) : standalone_(standalone) {}

WebsocketServer::~WebsocketServer() {
  server_.stop();
}

#ifdef _COMPILE_FOR_SERVER_
context_ptr on_tls_init(websocketpp::connection_hdl hdl) {
  context_ptr ctx = websocketpp::lib::make_shared<asio::ssl::context>
    (asio::ssl::context::sslv23);
  try {
    ctx->set_options(asio::ssl::context::default_workarounds |
      asio::ssl::context::no_sslv2 |
      asio::ssl::context::no_sslv3 |
      asio::ssl::context::no_tlsv1 |
      asio::ssl::context::single_dh_use);
    ctx->set_password_callback(bind(&get_password));
    ctx->use_certificate_chain_file("/etc/letsencrypt/live/kava-i.de-0001/fullchain.pem");
    ctx->use_private_key_file("/etc/letsencrypt/live/kava-i.de-0001/privkey.pem", 
        asio::ssl::context::pem);

    std::string ciphers;
    ciphers = "ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-RSA-CHACHA20-POLY1305:ECDHE-ECDSA-"
      "AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES256-GCM-SHA384:"
      "ECDHE-RSA-AES256-GCM-SHA384:DHE-RSA-AES128-GCM-SHA256:DHE-RSA-AES256-GCM-SHA384:"
      "ECDHE-ECDSA-AES128-SHA256:ECDHE-RSA-AES128-SHA256:ECDHE-ECDSA-AES128-SHA:ECDHE-"
      "RSA-AES256-SHA384:ECDHE-RSA-AES128-SHA:ECDHE-ECDSA-AES256-SHA384:ECDHE-ECDSA-"
      "AES256-SHA:ECDHE-RSA-AES256-SHA:DHE-RSA-AES128-SHA256:DHE-RSA-AES128-SHA:DHE-"
      "RSA-AES256-SHA256:DHE-RSA-AES256-SHA:ECDHE-ECDSA-DES-CBC3-SHA:ECDHE-RSA-DES-CBC3-"
      "SHA:EDH-RSA-DES-CBC3-SHA:AES128-GCM-SHA256:AES256-GCM-SHA384:AES128-SHA256:AES256-"
      "SHA256:AES128-SHA:AES256-SHA:DES-CBC3-SHA:!DSS";
    if (SSL_CTX_set_cipher_list(ctx->native_handle(), ciphers.c_str()) != 1) {
      spdlog::get(LOGGER)->error("WebsocketFrame::on_tls_init: error setting cipher list");
    }
  } catch (std::exception& e) {
    spdlog::get(LOGGER)->error("WebsocketFrame::on_tls_init: {}", e.what());
  }
  return ctx;
}
#endif


void WebsocketServer::Start(int port) {
  try { 
    spdlog::get(LOGGER)->info("WebsocketFrame::Start: set_access_channels");
    server_.set_access_channels(websocketpp::log::alevel::none);
    server_.clear_access_channels(websocketpp::log::alevel::frame_payload);
    server_.init_asio(); 
    server_.set_message_handler(bind(&WebsocketServer::on_message, this, &server_, ::_1, ::_2)); 
    #ifdef _COMPILE_FOR_SERVER_
      server_.set_tls_init_handler(bind(&on_tls_init, ::_1));
    #endif
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
  std::unique_lock ul(shared_mutex_connections_);
  if (connections_.count(hdl.lock().get()) == 0)
    connections_[hdl.lock().get()] = new Connection(hdl, hdl.lock().get(), "");
  else
    spdlog::get(LOGGER)->warn("WebsocketFrame::OnOpen: New connection, but connection already exists!");
}

void WebsocketServer::OnClose(websocketpp::connection_hdl hdl) {
  std::unique_lock ul(shared_mutex_connections_);
  if (connections_.count(hdl.lock().get()) > 0) {
    try {
      // Delete connection.
      std::string username = connections_.at(hdl.lock().get())->username();
      delete connections_[hdl.lock().get()];
      if (connections_.size() > 1)
        connections_.erase(hdl.lock().get());
      else 
        connections_.clear();
      ul.unlock();
      spdlog::get(LOGGER)->info("WebsocketFrame::OnClose: Connection closed successfully.");
      // Tell game that player has disconnected. 
      std::unique_lock ul_games(shared_mutex_games_);
      auto game = GetGameFromUsername(username);
      if (game)
        game->PlayerResigned(username);
    } catch (std::exception& e) {
      spdlog::get(LOGGER)->error("WebsocketFrame::OnClose: Failed to close connection: {}", e.what());
    }
  }
  else 
    spdlog::get(LOGGER)->info("WebsocketFrame::OnClose: Connection closed, but connection didn't exist!");
}

void WebsocketServer::on_message(server* srv, websocketpp::connection_hdl hdl, message_ptr msg) {
  spdlog::get(LOGGER)->debug("Websocket::on_message: new message");
  // Handle binary data (audio-files) in otherwise.
  if (msg->get_opcode() == websocketpp::frame::opcode::binary && connections_.count(hdl.lock().get())) {
    spdlog::get(LOGGER)->debug("Websocket::on_message: got binary data");
    std::string content = msg->get_payload(); 
    std::string filename = content.substr(0, content.find("$"));
    std::string path = "dissonance/data/user-files/"+connections_.at(hdl.lock().get())->username();
    if (!std::filesystem::exists(path))
      std::filesystem::create_directory(path);
    path += +"/"+filename;
    spdlog::get(LOGGER)->debug("Websocket::on_message: storing binary data to: {}", path);
    utils::StoreMedia(path, content.substr(content.find("$")+1));
    spdlog::get(LOGGER)->debug("Websocket::on_message: Done.");
    return;
  }
  // Validate json.
  spdlog::get(LOGGER)->debug("Websocket::on_message: validating new msg: {}", msg->get_payload());
  auto json_opt = utils::ValidateJson({"command", "username", "data"}, msg->get_payload());
  if (!json_opt.first) {
    spdlog::get(LOGGER)->warn("Server: message with missing required fields (required: command, username, data)");
    return;
  }
  std::string command = json_opt.second["command"];
  std::string username = json_opt.second["username"];
  
  // Create controller connection
  if (command == "initialize")
    h_InitializeUser(hdl.lock().get(), username, json_opt.second);
  // Start new game
  else if (command == "init_game")
    h_InitializeGame(hdl.lock().get(), username, json_opt.second);
  // Indicate that player is ready (audio downloaded).
  else if (command == "ready") {
    if (username_game_id_mapping_.count(username) > 0)
      games_.at(username_game_id_mapping_.at(username))->PlayerReady(username);
  }
  // In game action
  else
    h_InGameAction(hdl.lock().get(), username, json_opt.second);
}

void WebsocketServer::h_InitializeUser(connection_id id, std::string username, const nlohmann::json& msg) {
  // Create controller connection.
  // Username already exists.
  if (connections_.count(id) > 0 && UsernameExists(username)) {
    nlohmann::json resp = {{"command", "kill"}, {"data", {{"msg", "Username exists!"}}}};
    SendMessage(id, resp.dump());
  }
  // Game exists with current username
  else if (username_game_id_mapping_.count(username)) {
    nlohmann::json resp = {{"command", "kill"}, {"data", {{"msg", "A game for this username is currently running!"}}}};
    SendMessage(id, resp.dump());
  }
  // Everything fine: set username and send next instruction to user.
  else if (connections_.count(id) > 0) {
    std::unique_lock ul(shared_mutex_connections_);
    connections_[id]->set_username(username);
    ul.unlock();
    SendMessage(id, nlohmann::json({{"command", "select_mode"}}).dump());
  }
  else {
    spdlog::get(LOGGER)->warn("WebsocketServer::h_InitializeUser: connection does not exist! {}", username);
  }
}

void WebsocketServer::h_InitializeGame(connection_id id, std::string username, const nlohmann::json& msg) {
  nlohmann::json data = msg["data"];
  if (data["mode"] == SINGLE_PLAYER) {
    spdlog::get(LOGGER)->info("Server: initializing new single-player game.");
    std::string game_id = username;
    std::unique_lock ul(shared_mutex_games_);
    username_game_id_mapping_[username] = game_id;
    games_[game_id] = new ServerGame(data["lines"], data["cols"], data["mode"], 2, data["base_path"], this);
    SendMessage(id, nlohmann::json({{"command", "select_audio"}, {"data", nlohmann::json()}}).dump());
  }
  else if (data["mode"] == MULTI_PLAYER) {
    spdlog::get(LOGGER)->info("Server: initializing new multi-player game.");
    std::string game_id = username;
    std::unique_lock ul(shared_mutex_games_);
    username_game_id_mapping_[username] = game_id;
    games_[game_id] = new ServerGame(data["lines"], data["cols"], data["mode"], data["num_players"],
        data["base_path"], this);
    SendMessage(id, nlohmann::json({{"command", "select_audio"}, {"data", nlohmann::json()}}).dump());
  }
  else if (data["mode"] == MULTI_PLAYER_CLIENT) {
    std::unique_lock ul(shared_mutex_games_);
    bool found_game = false;
    for (const auto& it : games_) {
      if (it.second->status() == WAITING_FOR_PLAYERS) {
        username_game_id_mapping_[username] = it.first;  // Add username to mapping, to find matching game.
        it.second->AddPlayer(username, data["lines"], data["cols"]); // Add user to game.
        found_game = true;
        SendMessage(id, nlohmann::json({{"command", "print_msg"}, {"data", 
              {{"msg", "Waiting for other players"}}}}).dump());
        break;
      }
    }
    if (!found_game) {
      SendMessage(id, nlohmann::json({{"command", "print_msg"}, {"data", {{"msg", "No Game Found"}} }}).dump());
    }
  }
  else if (data["mode"] == OBSERVER) {
    spdlog::get(LOGGER)->info("Server: initializing new observer game.");
    std::string game_id = username;
    std::unique_lock ul(shared_mutex_games_);
    username_game_id_mapping_[username] = game_id;
    games_[game_id] = new ServerGame(data["lines"], data["cols"], data["mode"], 2, data["base_path"], this);
    SendMessage(id, nlohmann::json({{"command", "select_audio"}, {"data", nlohmann::json()}}).dump());
  }
}

void WebsocketServer::h_InGameAction(connection_id id, std::string username, const nlohmann::json& msg) {
  std::shared_lock sl(shared_mutex_games_);
  auto game = GetGameFromUsername(username);
  if (game) {
    nlohmann::json resp = game->HandleInput(msg["command"], msg);
    if (resp.contains("command")) {
      SendMessage(id, resp.dump());
      resp["username"] = "";
    }
  }
  else
    spdlog::get(LOGGER)->warn("Server: message with unkown command or username.");
}

void WebsocketServer::CloseGames() {
  for(;;) {
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    std::unique_lock ul(shared_mutex_games_);
    std::vector<std::string> games_to_delete;
    spdlog::get(LOGGER)->info("Checking running games");
    // Gather games to delete or close games if all users are disconnected.
    for (const auto& it : games_) {
      // If game is closed and to game to delete.
      if (it.second->status() == CLOSED)
        games_to_delete.push_back(it.first);
      // If all users are disconnected, set status to closed.
      else if (GetPlayingUsers(it.first, true).size() == 0)
        it.second->set_status((it.second->status() < SETTING_UP) ? CLOSED : CLOSING);
    }
    // Detelte games to delete, remove from games and remove all player-game-mappings for this game.
    for (const auto& it :games_to_delete) {
      // Delete game and remove from list of games.
      delete games_[it];
      games_.erase(it);
      // Remove all username-game-mappings
      std::vector<std::string> all_users = GetPlayingUsers(it);
      for (const auto& it : all_users)
        username_game_id_mapping_.erase(it);
      // Exit if standalone-server.
      if (!standalone_)
        exit(0);
    }
  }
}

WebsocketServer::connection_id WebsocketServer::GetConnectionIdByUsername(std::string username) {
  spdlog::get(LOGGER)->debug("WebsocketServer::GetConnectionIdByUsername: username: {}", username);
  for (const auto& it : connections_)
    if (it.second->username() == username) 
      return it.second->get_connection_id();
  return connection_id();
}

bool WebsocketServer::UsernameExists(std::string username) {
  spdlog::get(LOGGER)->debug("WebsocketServer::UsernameExists: username: {}", username);
  for (const auto& it : connections_)
    if (it.second->username() == username) 
      return true;
  return false;
}

ServerGame* WebsocketServer::GetGameFromUsername(std::string username) {
  if (username_game_id_mapping_.count(username) > 0 
      && games_.count(username_game_id_mapping_.at(username)))
    return games_.at(username_game_id_mapping_.at(username));
  return nullptr;
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

void WebsocketServer::SendMessage(std::string username, nlohmann::json msg) {
  spdlog::get(LOGGER)->debug("WebsocketFrame::SendMessage: username {}", username);
  SendMessage(GetConnectionIdByUsername(username), msg.dump());
}

void WebsocketServer::SendMessage(connection_id id, std::string msg) {
  try {
    // Set last incomming and get connection hdl from connection.
    std::shared_lock sl(shared_mutex_connections_);
    if (connections_.count(id) == 0) {
      spdlog::get(LOGGER)->error("WebsocketFrame::SendMessage: failed to get connection to {}", id);
      return;
    }
    // Send message.
    server_.send(connections_.at(id)->connection(), msg, websocketpp::frame::opcode::value::text);
    spdlog::get(LOGGER)->debug("WebsocketFrame::SendMessage: successfully sent message.");

  } catch (websocketpp::exception& e) {
    spdlog::get(LOGGER)->error("WebsocketFrame::SendMessage: failed sending message: {}", e.what());
  } catch (std::exception& e) {
    spdlog::get(LOGGER)->error("WebsocketFrame::SendMessage: failed sending message: {}", e.what());
  }
}

void WebsocketServer::SendMessageBinary(std::string username, std::string msg) {
  auto connection_id = GetConnectionIdByUsername(username);
  try {
    // Set last incomming and get connection hdl from connection.
    std::shared_lock sl(shared_mutex_connections_);
    if (connections_.count(connection_id) == 0) {
      spdlog::get(LOGGER)->error("WebsocketFrame::SendMessage: failed to get connection to {}", connection_id);
      return;
    }
    // Send message.
    server_.send(connections_.at(connection_id)->connection(), msg, websocketpp::frame::opcode::binary);
    spdlog::get(LOGGER)->debug("WebsocketFrame::SendMessage: successfully sent message.");
  } catch (websocketpp::exception& e) {
    spdlog::get(LOGGER)->error("WebsocketFrame::SendMessage: failed sending message: {}", e.what());
  } catch (std::exception& e) {
    spdlog::get(LOGGER)->error("WebsocketFrame::SendMessage: failed sending message: {}", e.what());
  }
}
