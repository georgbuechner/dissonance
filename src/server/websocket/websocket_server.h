/**
 * @author: fux
 */

#ifndef SERVER_WEBSOCKET_SERVER_H_
#define SERVER_WEBSOCKET_SERVER_H_

#include <chrono>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#define NCURSES_NOMACROS
#include <shared_mutex>
#include <string>
#include <websocketpp/common/connection_hdl.hpp>

#include "share/shemes/commands.h"
#include "share/shemes/data.h"
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
     * Constructs websocket-server.
     * @param[in] standalone
     * @param[in] base_path 
     */
    WebsocketServer(bool standalone, std::string base_path);

    /**
     * Destructor stoping websocket server
     */
    ~WebsocketServer();

    /**
     * Initializes and starts main loop. (THREAD)
     * @param[in] port 
     */
    void Start(int port);

    /**
     * Sends message to given connection by username.
     * @param username 
     * @param cmd reference to command 
     */
    void SendMessage(std::string username, Command& cmd);

    /**
     * Sends message to given username. 
     * Constucts data to send from `command` and `data`.
     */
    void SendMessage(std::string username, std::string command, std::shared_ptr<Data> data);

    /**
     * Sends message to given id. 
     * Constucts command to send from `command` and `data`.
     */
    void SendMessage(connection_id id, std::string command, std::shared_ptr<Data> data);

    /**
     * Closes games every 5 seconds if finished. (THREAD)
     * (if standalone_, closes thread)
     * Updates lobby if a game is closed.
     */
    void CloseGames();

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
    mutable std::shared_mutex mutex_connections_;  ///< Mutex for connections_.
    std::map<connection_id, Connection*> connections_;  ///< Dictionary with all connections.

    // mutex for games and games-map:
    // `mutex_games_map_` is alway unique-locked when games-map is modified (and also when a game is deleted)
    // The mutex should always be shared-locked, when map is accessed.
    // `mutex_games_` is only unique-locked when game is deleted (as adding a game to map is irrelevant to
    // any game-action, and deleting from map only happens, when the game is also deleted).
    // The mutex hast be be shared-locked only when a game-action is executed.
    // → This way a in-game-action blocks deleting a game, but also modifying the games-map is allowed during 
    // a game-action.
    mutable std::shared_mutex mutex_games_map_;  ///< Mutex for games map (sl when accessed, ul when modified)
    mutable std::shared_mutex mutex_games_;  ///< Mutex for games (sl when used, ul when games are deleted).
    std::map<std::string, std::string> username_game_id_mapping_;  ///< maps username to game
    std::map<std::string, ServerGame*> games_;  ///< all games (key=host-player).

    std::shared_ptr<Lobby> lobby_;  ///< the game-lobby.
    mutable std::shared_mutex mutex_lobby_;  ///< Mutex for connections_.

    const bool standalone_;  ///< if set, closes CloseGames-thread when a game is over.
    const std::string base_path_;

    // methods:
    
    /**
     * Opens a new connection and adds entry in dictionary of connections.
     * Until the connection is futher initialized it will be set to a
     * Connection. Later this is deleted and replaced with ControllerConnection,
     * UserOverviewConnection or UserControllerConnection.
     * @param hdl[in] websocket-connection.
     */
    void OnOpen(websocketpp::connection_hdl hdl);

    /**
     * Closes an active connection and removes connection from connections.
     * @param[in] hdl websocket-connection.
     */
    void OnClose(websocketpp::connection_hdl hdl);
        
    /**
     * Handles incomming requests/ commands.
     * @param[in] srv pointer to server.
     * @param[in] hdl incomming connection.
     * @param[in] msg message.
     */
    void on_message(server* srv, websocketpp::connection_hdl hdl, message_ptr msg);

    /**
     * Sends message to given connection by connection-id.
     * Shared-locks connections-mutex.
     * @param[in] id of connection over which to send.
     * @param[in] msg message which to send.
     */
    void SendMessage(connection_id id, std::string msg);

    /**
     * Gets player connection by username.
     * Shared-locks connections-mutex
     * @param[in] username
     * @return id of connection.
     */
    connection_id GetConnectionIdByUsername(std::string username);

    /**
     * Checks whether a username is already in use.
     * Shared-locks connections-mutex
     * @param[in] username
     * @return whether a connection with this username already exists.
     */
    bool UsernameExists(std::string username);

    /**
     * Gets game from username. Checks whether username and game exists. 
     * Shared-locks games-map- and connections-mutex
     * @param[in] username
     * @retrun game or nullptr if game or user does not exist.
     */
    ServerGame* GetGameFromUsername(std::string username);

    /**
     * Gets all users playing same game a user. 
     * No mutex locked (only called in `CloseGames` which locks most mutexes
     * anyway)
     * @param username
     * @param check_connected if set, only returns users which are currently connected
     * @return list of all users belonging to one game. Of `check_connected`
     * set, returns only currently online users.
     */
    std::vector<std::string> GetPlayingUsers(std::string username, bool check_connected=false);

    /**
     * Updates lobby, then sends updated lobby to all users in lobby
     * (No mutex locked, but `UpdateLobby` lockes games-map- and lobby-mutex and `SendMessage` 
     * locks connection-mutex)
     */
    void SendUpdatedLobbyToAllInLobby();

    /**
     * Updates lobby. 
     * Empties lobby first, then fills lobby with all game where status is WAITING_FOR_PLAYERS.
     * Unique-lock mutex lobby
     * Shared-lock mutex games
     */
    void UpdateLobby();

    // handlers:
    
    /**
     * Sets up new user-connection.
     * @param[in] id 
     * @param[in] username 
     * @param[in, out] msg
     */
    void h_InitializeUser(connection_id id, std::string username);

    /**
     * Starts new game in desired mode.
     * @param[in] id 
     * @param[in] username 
     * @param[in, out] msg
     */
    void h_InitializeGame(connection_id id, std::string username, std::shared_ptr<Data> data);

    /**
     * Calls game for this user, to handle in game action.
     * @param[in] id 
     * @param[in] username 
     * @param[in, out] msg
     */
    void h_InGameAction(connection_id id, Command cmd);
};

#endif 
