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
    int status() const;
    int mode() const;
    int max_players() const;
    int cur_players() const;
    std::string audio_map_name() const;

    // setter 
    void set_status(int status);

    // methods.
    
    /**
     * Sends message to all other players that player has lost (resigned) and
     * set player status to lost.
     * @param[in] username
     */
    void PlayerResigned(std::string username);

    /**
     * Adds new players and checks if game is ready to start.
     * @param[in] username 
     * @param[in] lines (availible lines of new player)
     * @param[in] cols (availible columns of new player)
     */
    void AddPlayer(std::string username, int lines, int cols);

    /**
     * Starts game once all players are ready.
     * @param[in] username
     */
    void PlayerReady(std::string username);

    /**
     * Handles input.
     * @param[in] command
     * @param[in] data
     */
    void HandleInput(std::string command, std::shared_ptr<Data> data);

    bool TestField(std::string audio_path);
    
  private: 
    // field
    Field* field_;  ///< field 
    int lines_; 
    int cols_;

    // players
    std::shared_mutex mutex_players_;
    std::map<std::string, Player*> players_;
    std::map<std::string, Player*> human_players_;  ///< easy acces for only human-players
    std::string host_;  ///< host player.
    std::vector<std::string> observers_;  ///< vector with usernames of observers
    std::set<std::string> dead_players_;  ///< keeps track of dead players.
    const int max_players_;

    // audio
    Audio audio_;  ///< main-audio (f.e. map and in single-player: ai)
    std::map<int, std::string> audio_data_sorted_buffer_;
    std::string audio_data_buffer_;  ///< audio data (used as buffer when receiving audio-data)
    std::string audio_file_name_;  ///< audio-file-name.
    std::string base_path_;
    bool audio_stored_;
    std::atomic<int> last_audio_part_;
    std::shared_mutex mutex_audio_;

    // meta 
    WebsocketServer* ws_server_;
    EventManager<std::string, ServerGame, std::shared_ptr<Data>> eventmanager_;
    const int mode_; ///< (see: codes.h: Mode)
    const bool tutorial_; ///< when true, less resources for enemy.
    std::shared_mutex mutex_status_;  ///< mutex locked, when printing field.
    int status_;  ///< current game status (see: codes.h: GameStatus)
    std::shared_mutex mutex_pause_;
    int pause_;  //< indicates whether game is currently paused (only availible in single-player-mode)
    std::chrono::time_point<std::chrono::steady_clock> pause_start_;  ///< start-time of pause
    double time_in_pause_;  ///< time in pause (used for finding next audio-beat correctly after pause)

    // methods

    // getter 
    
    /**
     * Gets vector of enemies fort given player.
     * @param[in] players 
     * @param[in] player
     * @return vector of enemies fort given player.
     */
    std::vector<Player*> enemies(const std::map<std::string, Player*>& players, std::string player) const;

    /**
     * Gets map of all potentials in stacked format (ipsp: 1-9, epsp a-z) and
     * "swallows" epsp if enemy ipsp is on same field.
     * @return map of potentials in stacked format.
     */
    std::map<position_t, std::pair<std::string, int>> GetAndUpdatePotentials() const;

    /**
     * Create player-agnostic transfer-data.
     * @param[in] audio_played between 0 and 1 indicating song progress.
     * @return player-agnostic transfer-data.
     */
    std::shared_ptr<Update> CreateBaseUpdate(float audio_played) const;

    /**
     * Creates transfer-data and sends it to all online players.
     * @param[in] audio_played between 0 and 1 indicating song progress.
     */
    void SendUpdate(float audio_played) const;

    /**
     * Creates initial transfer-data (includeing field and graph_positions) and sends 
     * it to all online players.
     */
    void SendInitialData() const;

    /**
     * Checks if new players have died. Eventually sends messages or finally closes game, 
     * when all players except of one have died.
     */
    void HandlePlayersLost();

    /**
     * Checks if player has loophols and sends loophol-field-positions to
     * player.
     */ 
    void SendLoopHols(std::string username, Player* player) const;

    /**
     * Checks if for any (human) player a potential has scouted new enemy
     * neurons. If so, send these to client.
     */
    void SendScoutedNeurons() const;

    /**
     * Sends all new neurons created to all "listening" observers.
     */
    void SendNeuronsToObservers() const;

    /**
     * Sends given message to all online players (ignoring AI). Possibilt to
     * ignore a specific user.
     * @param[in] message
     * @param[in] ignore_username if set, this user is ignored (default: not set)
     */
    void SendMessageToAllPlayers(Command cmd, std::string ignore_username="") const;

    /**
     * Creates string of missing costs.
     * @param[in] missing_costs
     * @return string with missing resources, or empty string.
     */
    static std::string GetMissingResourceStr(Costs missing_costs);

    // command methods

    /**
     * Either loads audios from disc (if same-device or audio already exists) or
     * requests audio-data from host.
     * @param[in] data (CheckSendAudio)
     */
    void m_SendAudioMap(std::shared_ptr<Data> data);

    /**
     * Sends audio-data to player.
     * @param[in] data (used only to get player's username).
     */
    void m_SendSong(std::shared_ptr<Data> data);

    /**
     * Adds new audio-data-part to audio-buffer.
     * @param[in] data (AudioTransferData)
     */
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

    /**
     * Activates pause.
     */
    void m_SetPauseOn(std::shared_ptr<Data>);

    /**
     * Deactivates pause.
     */
    void m_SetPauseOff(std::shared_ptr<Data>);

    /**
     * Build potential.
     * @param[in, out] msg
     */
    void BuildPotentials(int unit, position_t pos, int num_potenials_to_build, std::string username, Player* player);

    /** 
     * Returns pointer to player from username.
     * Throws if player not found.
     */
    Player* GetPlayer(std::string username) const;

    /**
     * Sets up game.
     * Calls SetUpField and creates players.
     * @param[in] audios.
     */
    void SetUpGame(std::vector<Audio*> audios);

    /**
     * Sets up field.
     * @param[in] ran_gen (random generator used for game).
     * @return list of nucleus-positions of players.
     */
    std::vector<position_t> SetUpField(RandomGenerator* ran_gen);

    /**
     * Runs game.
     * - sends start-game info with initial game data to all and players.
     * - starts main- and ai-thread(s)
     * @param[in] audios (audios-used for multiple ais. Default empty).
     */
    void RunGame(std::vector<Audio*> audios = {});

    // Threads

    /**
     * Updates game status at every beat. (THREAD)
     */
    void Thread_RenderField();

    /**
     * Handles AI actions at every beat.
     * @param[in] username of ai handled by this thread.
     */
    void Thread_Ai(std::string username);
};

#endif
