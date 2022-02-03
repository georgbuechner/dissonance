/**
 * @author: fux
 */

#ifndef SERVER_WEBSOCKET_SERVER_H_
#define SERVER_WEBSOCKET_SERVER_H_

#include <chrono>
#include <iostream>
#include <list>
#include <map>
#include <nlohmann/json.hpp>
#include <shared_mutex>
#include <string>
#include <websocketpp/common/connection_hdl.hpp>

#include "server/websocket/connection.h"
#include "server/game/server_game.h"
#include "share/defines.h"

#ifdef _COMPILE_FOR_SERVER_
#include <websocketpp/config/asio.hpp>
#else
#include <websocketpp/config/asio_no_tls.hpp>
#endif

#include <websocketpp/server.hpp>

#ifdef _COMPILE_FOR_SERVER_
typedef websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> context_ptr;
#endif

class WebsocketServer {

  public:
     ///< Connection_id-typedef.
    typedef decltype(websocketpp::lib::weak_ptr<void>().lock().get()) connection_id;

    // public methods:
   
    /**
     * Constructors websocket frame with pinter to user-manager.
     */
    WebsocketServer(bool standalone);

    /**
     * Destructor stoping websocket server
     */
    ~WebsocketServer();

    /**
     * Initializes and starts main loop.
     * @param[in] port (port an which to listen to: 4181)
     */
    void Start(int port);

    /**
     * Sends message to given connection by username.
     * @param id of connection over which to send.
     * @param msg message which to send.
     */
    void SendMessage(std::string username, std::string msg);

    /**
     * Sends message to given connection by username as binary data
     * @param id of connection over which to send.
     * @param msg message which to send.
     */
    void SendMessageBinary(std::string username, std::string msg);

  private:

    // typedefs:
#ifdef _COMPILE_FOR_SERVER_
    typedef websocketpp::server<websocketpp::config::asio_tls> server;
#else
    typedef websocketpp::server<websocketpp::config::asio> server;
#endif
    typedef server::message_ptr message_ptr;
    typedef std::pair<int, std::string> error_obj;
    
    // members:

    server server_;  ///< server object.
    mutable std::shared_mutex shared_mutex_connections_;  ///< Mutex for connections_.
    std::map<connection_id, Connection*> connections_;  ///< Dictionary with all connections.

    mutable std::shared_mutex shared_mutex_games_;  ///< Mutex for connections_.
    std::map<std::string, std::string> username_game_id_mapping_;
    std::map<std::string, ServerGame*> games_;

    const bool standalone_;

    // methods:
    
    /**
     * Opens a new connection and adds entry in dictionary of connections.
     * Until the connection is futher initialized it will be set to a
     * Connection. Later this is deleted and replaced with ControllerConnection,
     * UserOverviewConnection or UserControllerConnection.
     * @param hdl websocket-connection.
     */
    void OnOpen(websocketpp::connection_hdl hdl);

    /**
     * Closes an active connection and removes connection from connections.
     * @param hdl websocket-connection.
     */
    void OnClose(websocketpp::connection_hdl hdl);
        
    /**
     * Handles incomming requests/ commands.
     * @param srv pointer to server.
     * @param hdl incomming connection.
     * @param msg message.
     */
    void on_message(server* srv, websocketpp::connection_hdl hdl, message_ptr msg);

    /**
     * Sends message to given connection by connection-id.
     * @param id of connection over which to send.
     * @param msg message which to send.
     */
    void SendMessage(connection_id id, std::string msg);

    /**
     * Gets player connection by username.
     * @param[in] username
     * @return id of connection.
     */
    connection_id GetConnectionIdByUsername(std::string username);

    /**
     * Gets game from username. Checks whether username and game exists. 
     * Locks mutex_game_
     * @param[in] username
     * @retrun game or nullptr if game or user does not exist.
     */
    ServerGame* GetGameFromUsername(std::string username);

    /**
     * Gets all users playing same game a user.
     */
    std::vector<std::string> GetPlayingUsers(std::string username);

    // handlers:
    
    /**
     * Sets up new user-connection.
     * @param[in] id 
     * @param[in] username 
     * @param[in, out] msg
     */
    void h_InitializeUser(connection_id id, std::string username, const nlohmann::json& msg);

    /**
     * Starts new game in desired mode.
     * @param[in] id 
     * @param[in] username 
     * @param[in, out] msg
     */
    void h_InitializeGame(connection_id id, std::string username, const nlohmann::json& msg);

    /**
     * Calls game for this user, to handle in game action.
     * @param[in] id 
     * @param[in] username 
     * @param[in, out] msg
     */
    void h_InGameAction(connection_id id, std::string username, const nlohmann::json& msg);

    /**
     * Deletes game and removes game and players. 
     * @param[in] id 
     * @param[in] username 
     * @param[in, out] msg
     */
    void h_CloseGame(connection_id id, std::string username, const nlohmann::json&);
};

#endif 
