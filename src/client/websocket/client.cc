#include "client/websocket/client.h"
#include "nlohmann/json_fwd.hpp"
#include "share/objects/units.h"

#include "share/shemes/commands.h"
#include "share/tools/utils/utils.h"
#include "spdlog/spdlog.h"
#include "websocketpp/frame.hpp"
#include <memory>

Client::Client(ClientGame* game, std::string username, std::string base_path) : username_(username) {
    game_ = game;
    base_path_ = base_path;
  }

bool Client::same_device() {
  return same_device_;
}

void Client::Start(std::string address) {
  // same_device_ = address.find("localhost") != std::string::npos || address.find("127.0.0.1") != std::string::npos;
  same_device_ = false; // TODO (fux): CHANGE!
  try {
    // Set logging to be pretty verbose (everything except message payloads)
    c_.set_access_channels(websocketpp::log::alevel::none);
    c_.clear_access_channels(websocketpp::log::alevel::frame_payload);

    // Initialize ASIO
    c_.init_asio();

    // Register our message handler
    c_.set_message_handler(websocketpp::lib::bind(&Client::on_message, this, &c_, 
          std::placeholders::_1, std::placeholders::_2));
    c_.set_open_handler(websocketpp::lib::bind(&Client::on_open, this, std::placeholders::_1));

    websocketpp::lib::error_code ec;
    client::connection_ptr con = c_.get_connection(address, ec);
    if (ec) {
      std::cout << "could not create connection because: " << ec.message() << std::endl;
      return;
    }
    
    // Grab a handle for this connection so we can talk to it in a thread
    // safe manor after the event loop starts.
    hdl_ = con->get_handle();

    // Note that connect here only requests a connection. No network messages are
    // exchanged until the event loop starts running in the next line.
    c_.connect(con);

    // Start the ASIO io_service run loop
    // this will cause a single connection to be made to the server. c.run()
    // will exit when this connection is closed.
    spdlog::get(LOGGER)->info("WebsocketFrame:: successfully started client to: {}", address);
    c_.run();
  } catch (websocketpp::exception const & e) {
    std::cout << e.what() << std::endl;
  }
}

void Client::Stop() {
  c_.close(hdl_, websocketpp::close::status::normal, "");
  // sleep(1);
  c_.stop();
}

void Client::on_open(websocketpp::connection_hdl) {
  SendMessage("initialize_user", std::make_shared<Data>());
}

// This message handler will be invoked once for each incoming message. It
// prints the message and then sends a copy of the message back to the server.
void Client::on_message(client* c, websocketpp::connection_hdl hdl, message_ptr msg) {
  spdlog::get(LOGGER)->info("Client::on_message: new message");
  Command cmd;
  try {
    cmd = Command(msg->get_payload().c_str(), msg->get_payload().size());
    spdlog::get(LOGGER)->info("Client::on_message: new message with command: {}", cmd.command());
  }
  catch (std::exception& e) {
    spdlog::get(LOGGER)->warn("Client::on_message: failed parsing message: {}", e.what());
    return;
  }

  // Handle json-formatted data:
  std::thread handler([this, cmd]() { game_->HandleAction(cmd); });
  handler.detach();
  spdlog::get(LOGGER)->info("Client::on_message: exited");
}

void Client::SendMessage(std::string command, std::shared_ptr<Data> data) {
  Command cmd(command, username_, data);
  spdlog::get(LOGGER)->debug("Client::SendMessage: username: {}, command: {}", username_, command);

  // TODO (fux): add username)
  websocketpp::lib::error_code ec;
  c_.send(hdl_, cmd.bytes(), websocketpp::frame::opcode::binary, ec);
  spdlog::get(LOGGER)->info("Client::SendMessage: successfully sent message.");
  if (ec)
    std::cout << "Client: sending failed because: " << ec.message() << std::endl;
}
