#ifndef SRC_SERVER_GAME_H_
#define SRC_SERVER_GAME_H_

#include "nlohmann/json_fwd.hpp"
#include "share/eventmanager.h"
#define NCURSES_NOMACROS
#include <cstddef>
#include <curses.h>
#include <mutex>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <shared_mutex>
#include <vector>

#include "audio/audio.h"
#include "constants/texts.h"
#include "game/field.h"
#include "player/audio_ki.h"
#include "player/player.h"
#include "objects/units.h"
#include "random/random.h"

class WebsocketServer;

class ServerGame {
  public:
    /** 
     * Constructor initializing game with availible lines and columns.
     * @param[in] lines availible lines.
     * @param[in] cols availible cols
     * @param[in] srv pointer to websocket-server, to send messages.
     */
    ServerGame(int lines, int cols, int mode, std::string base_path, WebsocketServer* srv, 
        std::string usr1, std::string usr2="");

    // getter 
    int status();

    // setter 
    void set_status(int status);

    // methods.

    /**
     * Handls input
     * @param[in] command
     * @param[in] msg
     * @return json to forward to user, if "command" is contained.
     */
    nlohmann::json HandleInput(std::string command, nlohmann::json msg);
    
    // Threads

    /**
     * Updates game status at every beat.
     */
    void Thread_RenderField();

    /**
     * Handles AI actions at every beat.
     */
    void Thread_Ai();

  private: 
    Field* field_;  ///< field 
    Player* player_one_;
    Player* player_two_;
    Audio audio_;
    WebsocketServer* ws_server_;
    EventManager<std::string, ServerGame, nlohmann::json&> eventmanager_;
    const std::string usr1_id_;
    const std::string usr2_id_;

    std::shared_mutex mutex_status_;  ///< mutex locked, when printing field.
    int status_;

    const int mode_; ///< SINGLE_PLAYER | MULTI_PLAYER | OBSERVER
    const int lines_;
    const int cols_;

    // handlers

    /**
     * Initializes game
     * @param[in] data 
     * @return command (json)
     */
    nlohmann::json InitializeGame(nlohmann::json data);

    // command methods

    /**
     * Analyzes audio.
     * @param[in, out] msg
     */
    void m_AnalyzeAudio(nlohmann::json& msg);

    /**
     * Adds iron
     * @param[in, out] msg
     */
    void m_AddIron(nlohmann::json& msg);

    /**
     * Removes iron
     * @param[in, out] msg
     */
    void m_RemoveIron(nlohmann::json& msg);

    /**
     * Adds technology
     * @param[in, out] msg
     */
    void m_AddTechnology(nlohmann::json& msg);
    /**
     * Handles if a player resignes
     * @param[in, out] msg
     */
    void m_Resign(nlohmann::json& msg);
};

#endif
