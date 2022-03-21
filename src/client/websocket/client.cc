#include "client/websocket/client.h"
#include "nlohmann/json_fwd.hpp"
#include "share/objects/units.h"

#include "share/tools/utils/utils.h"
#include "spdlog/spdlog.h"
#include "websocketpp/frame.hpp"

Client::Client(ClientGame* game, std::string username) : username_(username) {
    game_ = game;
  }

bool Client::same_device() {
  return same_device_;
}

void Client::Start(std::string address) {
  same_device_ = address.find("localhost") != std::string::npos || address.find("127.0.0.1") != std::string::npos;
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
  nlohmann::json msg = {{"command", "initialize"}, {"username", username_}, {"data", nlohmann::json()}};
  SendMessage(msg.dump());
}

// This message handler will be invoked once for each incoming message. It
// prints the message and then sends a copy of the message back to the server.
void Client::on_message(client* c, websocketpp::connection_hdl hdl, message_ptr msg) {
  // Handle binary data (audio-files) in otherwise.
  if (msg->get_opcode() == websocketpp::frame::opcode::binary) {
    spdlog::get(LOGGER)->debug("Websocket::on_message: got binary data");
    std::string content = msg->get_payload(); 
    auto metadata = utils::Split(content.substr(0, content.find("$")), "&");
    std::string username = metadata[0];
    std::string filename = metadata[1];
    std::string path = game_->base_path() + "/data/user-files/"+username;
    if (!std::filesystem::exists(path))
      std::filesystem::create_directory(path);
    path += +"/"+filename;
    spdlog::get(LOGGER)->debug("Websocket::on_message: storing binary data to: {}", path);
    utils::StoreMedia(path, content.substr(content.find("$")+1));
    game_->set_audio_file_path(path);
    spdlog::get(LOGGER)->debug("Websocket::on_message: Done.");
    SendMessage(nlohmann::json({{"command", "ready"}, {"username", username_}, {"data", nlohmann::json()}}).dump());
    return;
  }
  // Handle json-formatted data:
  spdlog::get(LOGGER)->debug("Client got message: {}", msg->get_payload());
  websocketpp::lib::error_code ec;
  nlohmann::json resp = game_->HandleAction(nlohmann::json::parse(msg->get_payload()));
  // If data is contained in response, add username and send.
  if (resp.contains("data")) {
    resp["username"] = username_;
    spdlog::get(LOGGER)->debug("About to send message:");
    spdlog::get(LOGGER)->debug("{}", resp.dump());
    SendMessage(resp.dump());
  }
}

void Client::SendMessage(std::string msg) {
  websocketpp::lib::error_code ec;
  spdlog::get(LOGGER)->debug("Client::SendMessage: {}", msg);
  c_.send(hdl_, msg, websocketpp::frame::opcode::text, ec);
  if (ec)
    std::cout << "Client: sending failed because: " << ec.message() << std::endl;
}

void Client::SendMessageBinary(std::string msg) {
  websocketpp::lib::error_code ec;
  spdlog::get(LOGGER)->debug("Client::SendMessage: sending binary message: size {}", msg.size());
  c_.send(hdl_, msg, websocketpp::frame::opcode::binary, ec);
  spdlog::get(LOGGER)->debug("Client::SendMessage: sending binary success", msg.size());
  if (ec)
    std::cout << "Client: sending failed because: " << ec.message() << std::endl;
}
