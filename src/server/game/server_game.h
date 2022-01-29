#ifndef SRC_SERVER_GAME_H_
#define SRC_SERVER_GAME_H_

#include "share/defines.h"
#define NCURSES_NOMACROS
#include <cstddef>
#include <curses.h>
#include <mutex>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <shared_mutex>
#include <vector>

#include "share/audio/audio.h"
#include "share/constants/texts.h"
#include "server/game/field/field.h"
#include "server/game/player/audio_ki.h"
#include "server/game/player/player.h"
#include "share/objects/units.h"
#include "share/tools/random/random.h"
#include "share/tools/eventmanager.h"

class WebsocketServer;

class ServerGame {
  public:
    /** 
     * Constructor initializing game with availible lines and columns.
     * @param[in] lines availible lines.
     * @param[in] cols availible cols
     * @param[in] srv pointer to websocket-server, to send messages.
     */
    ServerGame(int lines, int cols, int mode, int num_players, std::string base_path, WebsocketServer* srv);

    // getter 
    int status();
    int mode();

    // setter 
    void set_status(int status);

    // methods.
    

    /**
     *
     */
    void InitAiGame(std::string base_path, std::string path_audio_map, std::string path_audio_a, 
        std::string path_audio_b);

    /**
     * Prints statistics to stdout for all players.
     */
    void PrintStatistics();

    /**
     * Adds new players and checks if game is ready to start.
     * @param[in] username 
     */
    void AddPlayer(std::string username, int lines, int cols);

    /**
     * Handles input.
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
    void Thread_Ai(std::string username);

    bool RunAiGame();

  private: 
    Field* field_;  ///< field 
    std::shared_mutex mutex_players_;
    std::map<std::string, Player*> players_;
    std::map<std::string, Player*> human_players_;


    std::vector<std::string> observers_;
    std::set<std::string> dead_players_;
    const unsigned int num_players_;
    Audio audio_;
    WebsocketServer* ws_server_;
    EventManager<std::string, ServerGame, nlohmann::json&> eventmanager_;

    std::shared_mutex mutex_status_;  ///< mutex locked, when printing field.
    int status_;

    const int mode_; ///< SINGLE_PLAYER | MULTI_PLAYER | OBSERVER
    int lines_; 
    int cols_;

    // methods

    /**
     * Starts game, sending start-game info with initial game data to all
     * players.
     */
    void StartGame(std::vector<Audio*> audios);
    std::vector<position_t> SetUpField(RandomGenerator* ran_gen);

    /**
     * Gets map of all potentials in stacked format (ipsp: 1-9, epsp a-z) and
     * "swallows" epsp if enemy ipsp is on same field.
     * @return map of potentials in stacked format.
     */
    std::map<position_t, std::pair<std::string, int>> GetAndUpdatePotentials();

    /**
     * Create transfer data and sends it to all online players.
     * @param[in] audio_played between 0 and 1 indicating song progress.
     * @param[in] update indicating whether to send update or inital data. (default true)
     */
    void CreateAndSendTransferToAllPlayers(float audio_played, bool update=true);

    /**
     * Checks if new players have died. Eventually sends messages or finally closes game, 
     * when all players except of one have died.
     */
    void HandlePlayersLost();

    /**
     * Checks if for any (human) player a potential has scouted new enemy
     * neurons. If so, send these to client.
     */
    void SendScoutedNeurons();

    /**
     * Sends all new neurons created to all "listening" observers.
     */
    void SendNeuronsToObservers();

    /**
     * Sends given message to all online players (ignoring AI). Possibilt to
     * ignore a specific user.
     * @param[in] message
     * @param[in] ignore_username if set, this user is ignored (default: not set)
     */
    void SendMessageToAllPlayers(std::string msg, std::string ignore_username="");

    /**
     * Creates string of missing costs.
     * @param[in] missing_costs
     * @return string with missing resources, or empty string.
     */
    static std::string GetMissingResourceStr(Costs missing_costs);

    /**
     * If a enemy epsp is on same field as given ipsp, increase ipsp potential and decrease epsp 
     * potential by one.
     * @param[in] ipsp_pos position of ipsp in question.
     * @param[in] player who "owns" ipsp.
     * @param[in] list of enemies of given player.
     */
    static void IpspSwallow(position_t ipsp_pos, Player* player, std::vector<Player*> enemies);

    // command methods

    /**
     * Analyzes audio and initialize game.
     * @param[in, out] msg
     */
    void m_InitializeGame(nlohmann::json& data);

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

    /**
     * Checks if resources (or other criteria) are met for building neuron.
     * @param[in, out] msg
     */
    void m_CheckBuildNeuron(nlohmann::json& msg);

    /**
     * Checks if resources (or other criteria) are met for building potential.
     * @param[in, out] msg
     */
    void m_CheckBuildPotential(nlohmann::json& msg);

    /**
     * Build neuron.
     * @param[in, out] msg
     */
    void m_BuildNeurons(nlohmann::json& msg);

    /**
     * Gets all requested positions: might be of own- or enemy-units. Sends command given command
     * at msg["data"]["return_cmd"]
     * @param[in, out] msg
     */
    void m_GetPositions(nlohmann::json& msg);

    /**
     * Toggles swarm-attack.
     * @param[in, out] msg
     */
    void m_ToggleSwarmAttack(nlohmann::json& msg);

    /**
     * Set Way Point
     * @param[in, out] msg
     */
    void m_SetWayPoint(nlohmann::json& msg);

    /**
     * Set Ipsp target.
     * @param[in, out] msg
     */
    void m_SetIpspTarget(nlohmann::json& msg);

    /**
     * Set Epsp target.
     * @param[in, out] msg
     */
    void m_SetEpspTarget(nlohmann::json& msg);

    /**
     * Build potential.
     * @param[in, out] msg
     */
    void BuildPotentials(int unit, position_t pos, int num_potenials_to_build, std::string username, Player* player);
};

#endif
