#include <spdlog/spdlog.h>

#include "client/websocket/client.h"
#include "share/objects/units.h"
#include "share/shemes/commands.h"
#include "share/tools/utils/utils.h"

Client::Client(ClientGame* game, std::string username, std::string base_path) : username_(username) {
  game_ = game;
  base_path_ = base_path;
  closed_by_me_ = false;
}

void Client::Start(std::string address) {
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

    spdlog::get(LOGGER)->info("Client::Start: starting connection to {}", address);
    websocketpp::lib::error_code ec;
    client::connection_ptr con = c_.get_connection(address, ec);
    if (ec) {
      spdlog::get(LOGGER)->error("Client::Start: could not create connection because: {}", ec.message());
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
    spdlog::get(LOGGER)->info("Client::Start: successfully started client to: {}", address);
    c_.run();
  } catch (websocketpp::exception const& e) {
    spdlog::get(LOGGER)->error("Client::Start: websocket-error in event loop: {}", e.what());
    game_->Kill("Game crashed. We're sorry. Consider filing a bug report at \n"
      "https://github.com/georgbuechner/dissonance/issues");
  } catch (std::exception const& e) {
    spdlog::get(LOGGER)->error("Client::Start: error in event loop: {}", e.what());
    game_->Kill("Game crashed. We're sorry. Consider filing a bug report at \n"
      "https://github.com/georgbuechner/dissonance/issues");
  } catch (...) {
    spdlog::get(LOGGER)->error("Client::Start: unkown error in event loop.");
  }
  spdlog::get(LOGGER)->info("Client::Start. closed.");
  if (!closed_by_me_)
    game_->Kill("The websocketpp server has closed. Probably no connection to server exists. Try again?");
}

void Client::Stop() {
  c_.close(hdl_, websocketpp::close::status::normal, "");
  c_.stop();
  closed_by_me_ = true;
}

void Client::on_open(websocketpp::connection_hdl) {
  spdlog::get(LOGGER)->info("Client::on_open");
  SendMessage("initialize_user", std::make_shared<Data>());
}

// This message handler will be invoked once for each incoming message. It
// prints the message and then sends a copy of the message back to the server.
void Client::on_message(client* c, websocketpp::connection_hdl hdl, message_ptr msg) {
  Command cmd;
  try {
    cmd = Command(msg->get_payload().c_str(), msg->get_payload().size());
    spdlog::get(LOGGER)->debug("Client::on_message: new message with command: {}", cmd.command());
  }
  catch (std::exception& e) {
    spdlog::get(LOGGER)->warn("Client::on_message: failed parsing message: {}", e.what());
    return;
  }

  // Handle json-formatted data:
  std::thread handler([this, cmd]() { game_->HandleAction(cmd); });
  handler.detach();
  spdlog::get(LOGGER)->debug("Client::on_message: exited");
}

void Client::SendMessage(std::string command, std::shared_ptr<Data> data) {
  Command cmd(command, username_, data);
  spdlog::get(LOGGER)->debug("Client::SendMessage: username: {}, command: {}", username_, command);
  websocketpp::lib::error_code ec;
  c_.send(hdl_, cmd.bytes(), websocketpp::frame::opcode::binary, ec);
  spdlog::get(LOGGER)->debug("Client::SendMessage: successfully sent message.");
  if (ec)
    std::cout << "Client: sending failed because: " << ec.message() << std::endl;
}
