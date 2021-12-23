#ifndef SERVER_CONNECTION_H
#define SERVER_CONNECTION_H

#include <iostream>
#include <websocketpp/common/connection_hdl.hpp>

class Connection {
  public:
    ///< Connection_id-typedef.
    typedef decltype(websocketpp::lib::weak_ptr<void>().lock().get()) connection_id;

    Connection(websocketpp::connection_hdl connection, connection_id id, std::string username) 
        : connection_(connection), connection_id_(id) {
      waiting_ = false;
      closed_ = false;
    }


    // getter 
    websocketpp::connection_hdl connection() {
      return connection_;
    }

    connection_id get_connection_id() {
      return connection_id_;
    }

    std::string username() {
      return username_;
    }

    bool waiting() {
      return waiting_;
    }

    bool closed() {
      return closed_;
    }

    // setter
    
    void set_username(std::string username) {
      username_ = username;
    }

    void set_closed(bool closed) {
      closed_ = closed;
    }

  private:
    const websocketpp::connection_hdl connection_; ///< connection to send to.
    const connection_id connection_id_; ///< connection identifier.
    std::string username_;
    bool waiting_;
    bool closed_;
};

#endif
