/*
 * @author: georgbuechner
 */
#include <chrono>
#include <exception>
#include <iostream>
#include <mutex>
#include <ostream>
#include <shared_mutex>
#include <stdexcept>
#include <string>
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
#include "server/server_game.h"
#include "spdlog/common.h"
#include "spdlog/logger.h"
#include "websocket_server.h"
#include "nlohmann/json.hpp"

#include "utils/utils.h"
#include "websocketpp/close.hpp"
#include "websocketpp/common/system_error.hpp"

#include "constants/codes.h"

#define LOGGER "logger"

#ifdef _COMPILE_FOR_SERVER_
namespace asio = websocketpp::lib::asio;
#endif

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

//Function only needed for callback, but nether actually used.
inline std::string get_password() { return ""; }

WebsocketServer::WebsocketServer() {}

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

void WebsocketServer::StartGameRoutines() {
  std::map<std::string, std::pair<std::thread, std::thread>> games_running_;
  while(true) {
    for (const auto& it : games_) {
      if (games_running_.count(it.first) == 0) {
        if (it.second->status() == 1) {
          games_running_[it.first] = { 
            std::thread([it]() { it.second->Thread_RenderField(); }),
            std::thread([it]() { it.second->Thread_Ai(); })
          };
        }
      }
    }
  }
}

void WebsocketServer::OnOpen(websocketpp::connection_hdl hdl) {
  std::unique_lock ul(shared_mutex_connections_);
  if (connections_.count(hdl.lock().get()) == 0) {
    // spdlog::get(LOGGER)->info("WebsocketFrame::OnOpen: New connectionen added.");
    connections_[hdl.lock().get()] = new Connection(hdl, hdl.lock().get(), "");
  }
  else
    spdlog::get(LOGGER)->warn("WebsocketFrame::OnOpen: New connection, but connection already exists!");
}

void WebsocketServer::OnClose(websocketpp::connection_hdl hdl) {
  std::unique_lock ul(shared_mutex_connections_);
  if (connections_.count(hdl.lock().get()) > 0) {
    try {
      delete connections_[hdl.lock().get()];
      connections_.erase(hdl.lock().get());
      spdlog::get(LOGGER)->info("WebsocketFrame::OnClose: Connection closed successfully.");
    } catch (std::exception& e) {
      spdlog::get(LOGGER)->error("WebsocketFrame::OnClose: Failed to close connection: {}", e.what());
    }
  }
  else 
    spdlog::get(LOGGER)->info("WebsocketFrame::OnClose: Connection closed, but connection didn't exist!");
}

void WebsocketServer::on_message(server* srv, websocketpp::connection_hdl hdl, message_ptr msg) {
  spdlog::get(LOGGER)->info("Server: Got msg: {}", msg->get_payload());

  // Validate json.
  auto json_opt = utils::ValidateJson({"command", "username", "data"}, msg->get_payload());
  if (!json_opt.first)
    return;
  std::string command = json_opt.second["command"];
  std::string username = json_opt.second["username"];
  nlohmann::json data = json_opt.second["data"];
  
  // Check whether connection existings and print received command.
  if (command == "initialize") {
    // Create controller connection.
    std::unique_lock ul(shared_mutex_connections_);
    connections_[hdl.lock().get()]->set_username(username);
    ul.unlock();
    nlohmann::json resp = {{"command", "select_mode"}};
    SendMessage(hdl.lock().get(), resp.dump());
  }
  else if (command == "init_game") {
    if (json_opt.second["data"]["mode"] == SINGLE_PLAYER) {
      games_[username] = new ServerGame(data["lines"], data["cols"], data["mode"], data["base_path"],
          this, username);
      nlohmann::json resp = {{"command", "select_audio"}, {"data", nlohmann::json()}};
      SendMessage(hdl.lock().get(), resp.dump());
    }
  }
  else {
    if (games_.count(username)) {
      nlohmann::json resp = games_.at(username)->HandleInput(command, username, data);
      SendMessage(hdl.lock().get(), resp.dump());
    }
  }
}

WebsocketServer::connection_id WebsocketServer::GetConnectionIdByUsername(std::string username) {
  spdlog::get(LOGGER)->info("WebsocketServer::GetConnectionIdByUsername: username: {}", username);
  for (const auto& it : connections_)
    if (it.second->username() == username) 
      return it.second->get_connection_id();
  return connection_id();
}

void WebsocketServer::SendMessage(connection_id id, std::string msg) {
  try {
    // Set last incomming and get connection hdl from connection.
    std::shared_lock sl(shared_mutex_connections_);
    if (connections_.count(id) == 0) {
      spdlog::get(LOGGER)->error("WebsocketFrame::SendMessage: failed to get connection to {}", id);
      return;
    }
    spdlog::get(LOGGER)->info("WebsocketFrame::SendMessage: sending message to {}.", 
        connections_.at(id)->username());
    websocketpp::connection_hdl hdl = connections_.at(id)->connection();
    sl.unlock();

    // Send message.
    server_.send(hdl, msg, websocketpp::frame::opcode::value::text);
    spdlog::get(LOGGER)->debug("WebsocketFrame::SendMessage: successfully sent message.");
  } catch (websocketpp::exception& e) {
    spdlog::get(LOGGER)->error("WebsocketFrame::SendMessage: failed sending message: {}", e.what());
  } catch (std::exception& e) {
    spdlog::get(LOGGER)->error("WebsocketFrame::SendMessage: failed sending message: {}", e.what());
  }
}
