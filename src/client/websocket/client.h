#ifndef SRC_CLIENT_CLIENT_H_
#define SRC_CLIENT_CLIENT_H_

#include <iostream>
#include <unistd.h>
#include <string>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>
#include <nlohmann/json.hpp>

#include "client/game/client_game.h"
#include "nlohmann/json.hpp"
#include "websocketpp/common/functional.hpp"


class Client {
  public:
    Client(ClientGame* game, std::string username);

    // getter 
    bool same_device();

    // methods
    void Start(std::string address);
    void Stop();
    void SendMessage(std::string msg);
    void SendMessageBinary(std::string msg);

  private:
    typedef websocketpp::client<websocketpp::config::asio_client> client;
    typedef websocketpp::config::asio_client::message_type::ptr message_ptr;
    const std::string username_;
    client c_;
    bool same_device_;
    websocketpp::connection_hdl hdl_;
    ClientGame* game_;

    void on_open(websocketpp::connection_hdl);
    
    // This message handler will be invoked once for each incoming message. It
    // prints the message and then sends a copy of the message back to the server.
    void on_message(client* c, websocketpp::connection_hdl hdl, message_ptr msg);
};

#endif
