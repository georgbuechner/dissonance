#ifndef SRC_SERVER_GAME_H_
#define SRC_SERVER_GAME_H_

#include <cstddef>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <shared_mutex>
#include <vector>

#include "share/defines.h"
#include "share/audio/audio.h"
#include "share/constants/texts.h"
#include "server/game/field/field.h"
#include "server/game/player/audio_ki.h"
#include "server/game/player/player.h"
#include "share/objects/units.h"
#include "share/shemes/commands.h"
#include "share/shemes/data.h"
#include "share/tools/random/random.h"
#include "share/tools/eventmanager.h"

class WebsocketServer;
class AudioTransferData;

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
    int max_players();
    int cur_players();
    std::string audio_map_name();

    // setter 
    void set_status(int status);

    // methods.
    
    /**
     *
     */
    void PlayerResigned(std::string username);

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

    void PlayerReady(std::string username);

    /**
     * Handles input.
     * @param[in] command
     * @param[in] data
     */
    void HandleInput(std::string command, std::shared_ptr<Data> data);
    
    // Threads

    /**
     * Updates game status at every beat.
     */
    void Thread_RenderField();

    /**
     * Handles AI actions at every beat.
     */
    void Thread_Ai(std::string username);



  private: 
    struct McNode {
      int _count;
      float cur_weight_;
      std::map<std::string, McNode*> _nodes;

      void update(float weight) {
        _count++;
        cur_weight_ += weight;
      }
    };

    Field* field_;  ///< field 
    std::shared_mutex mutex_players_;
    std::map<std::string, Player*> players_;
    std::map<std::string, Player*> human_players_;

    std::vector<std::string> observers_;
    std::set<std::string> dead_players_;
    const unsigned int max_players_;
    unsigned int cur_players_;
    Audio audio_;
    std::string audio_data_;
    std::string audio_file_name_;
    std::string base_path_;

    WebsocketServer* ws_server_;
    EventManager<std::string, ServerGame, std::shared_ptr<Data>> eventmanager_;

    std::shared_mutex mutex_status_;  ///< mutex locked, when printing field.
    int status_;
    std::shared_mutex mutex_pause_;
    int pause_;
    std::chrono::time_point<std::chrono::steady_clock> pause_start_;
    double time_in_pause_;

    struct TimeAnalysis {
      long double total_time_in_game_;
      long double time_ai_action_;
      long double time_ai_ran_;
      long double time_ai_mc_;
      long double time_game_;
      long double time_setup_;
      TimeAnalysis() : total_time_in_game_(0), time_ai_action_(0), time_ai_mc_(0), time_game_(0), time_setup_(0) {}
      void print() { 
        std::cout << "- total time: " << total_time_in_game_/1000000 << std::endl;
        std::cout << "- ai action: " << time_ai_action_/1000000 << std::endl;
        std::cout << "  - ran action: " << time_ai_ran_/1000000 << std::endl;
        std::cout << "  - mc action: " << time_ai_mc_/1000000 << std::endl;
        std::cout << "- game: " << time_game_/1000000 << std::endl;
        std::cout << "- setup mc game: " << time_setup_/1000000 << std::endl;
      }
    };
    TimeAnalysis time_analysis_;

    const int mode_; ///< SINGLE_PLAYER | MULTI_PLAYER | OBSERVER
    int lines_; 
    int cols_;
    bool tutorial_;

    std::string host_;

    // methods

    /**
     * Gets map of all potentials in stacked format (ipsp: 1-9, epsp a-z) and
     * "swallows" epsp if enemy ipsp is on same field.
     * @return map of potentials in stacked format.
     */
    std::map<position_t, std::pair<std::string, short>> GetAndUpdatePotentials();

    /**
     * Create player-agnostic transfer-data.
     * @param[in] audio_played between 0 and 1 indicating song progress.
     * @return player-agnostic transfer-data.
     */
    std::shared_ptr<Update> CreateBaseUpdate(float audio_played);

    /**
     * Creates transfer-data and sends it to all online players.
     * @param[in] audio_played between 0 and 1 indicating song progress.
     */
    void SendUpdate(float audio_played);

    /**
     * Creates initial transfer-data (includeing field and graph_positions) and sends 
     * it to all online players.
     */
    void SendInitialData();

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
    void SendMessageToAllPlayers(Command cmd, std::string ignore_username="");

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
    // command methods

    void m_SendAudioMap(std::shared_ptr<Data> data);
    void m_SendSong(std::shared_ptr<Data> data);
    void m_AddAudioPart(std::shared_ptr<Data> data);

    /**
     * Analyzes audio and initialize game.
     * @param[in, out] data
     */
    void m_InitializeGame(std::shared_ptr<Data> data);

    /**
     * Adds iron
     * @param[in, out] data
     */
    void m_AddIron(std::shared_ptr<Data> data);

    /**
     * Removes iron
     * @param[in, out] data
     */
    void m_RemoveIron(std::shared_ptr<Data> data);

    /**
     * Adds technology
     * @param[in, out] data
     */
    void m_AddTechnology(std::shared_ptr<Data> data);

    /**
     * Checks if resources (or other criteria) are met for building neuron.
     * @param[in, out] data
     */
    void m_CheckBuildNeuron(std::shared_ptr<Data> data);

    /**
     * Checks if resources (or other criteria) are met for building potential.
     * @param[in, out] data
     */
    void m_CheckBuildPotential(std::shared_ptr<Data> data);

    /**
     * Build neuron.
     * @param[in, out] data
     */
    void m_BuildNeurons(std::shared_ptr<Data> data);

    /**
     * Gets all requested positions: might be of own- or enemy-units. Sends command given command
     * at data["data"]["return_cmd"]
     * @param[in, out] data
     */
    void m_GetPositions(std::shared_ptr<Data> data);

    /**
     * Toggles swarm-attack.
     * @param[in, out] data
     */
    void m_ToggleSwarmAttack(std::shared_ptr<Data> data);

    /**
     * Set Way Point
     * @param[in, out] data
     */
    void m_SetWayPoint(std::shared_ptr<Data> data);

    /**
     * Set Ipsp target.
     * @param[in, out] data
     */
    void m_SetTarget(std::shared_ptr<Data> data);

    void m_SetPauseOn(std::shared_ptr<Data> data);
    void m_SetPauseOff(std::shared_ptr<Data> data);


    /**
     * Build potential.
     * @param[in, out] msg
     */
    void BuildPotentials(int unit, position_t pos, int num_potenials_to_build, std::string username, Player* player);

    /** 
     * Returns pointer to player from username.
     * Throws if player not found.
     */
    Player* GetPlayer(std::string username);

    /**
     * Setups game
     */
    void SetUpGame(std::vector<Audio*> audios);
    std::vector<position_t> SetUpField(RandomGenerator* ran_gen);
    /**
     * Runs game, 
     * sends start-game info with initial game data to all and players.
     * starts main- and ai-thread(s)
     */
    void RunGame(std::vector<Audio*> audios = {});
    bool RunAiGame(std::vector<Audio*> audios = {});
    void RunMCGames(std::deque<AudioDataTimePoint> data_per_beat, McNode* node);
};

#endif
