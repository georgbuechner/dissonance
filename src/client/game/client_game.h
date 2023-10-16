#ifndef SRC_CLIENT_CLIENT_GAME_H_
#define SRC_CLIENT_CLIENT_GAME_H_

#define NCURSES_NOMACROS
#include <curses.h>
#include <shared_mutex>
#include <string>
#include <vector>

#include "client/game/print/drawrer.h"
#include "share/tools/eventmanager.h"
#include "share/tools/context.h"
#include "share/defines.h"
#include "share/audio/audio.h"
#include "share/constants/texts.h"
#include "share/shemes/commands.h"
#include "share/shemes/data.h"
#include "share/tools/audio_receiver.h"

class Client;

class ClientGame {
  public:

    /** 
     * Constructor initializing basic settings and ncurses.
     * @param[in] audio_base_path
     */
    ClientGame(std::string base_path, std::string username, bool muliplayer_availible);

    // setter 
    void set_client(Client* ws_srv);

    // methods 

    /**
     * Handles action from server.
     * @param[in] cmd command comming from server.
     */
    void HandleAction(Command cmd);

    /**
     * Gets action from player.
     */
    void GetAction();

    /**
     * Calls h_Kill. Used to call from outside of class.
     */
    void Kill(std::string msg);

    /**
     * Initializes handlers.
     */
    static void init();

  private: 
    // member variables.
    const std::string username_;
    const bool muliplayer_availible_;
    Client* ws_srv_;
    std::shared_mutex mutex_;  ///< mutex locked, when setting transfer.

    // eventmanager
    static std::map<int, std::map<char, void(ClientGame::*)(std::shared_ptr<Data> data)>> handlers_;
    EventManager<std::string, ClientGame, std::shared_ptr<Data>> eventmanager_;
    EventManager<std::string, ClientGame, std::shared_ptr<Data>> eventmanager_tutorial_;

    std::shared_mutex mutex_tutorial_;  ///< mutex locked, when tutorial text is printed.
    bool in_tutorial_;

    /**
     * Stores "story-points" for tutorial
     */
    struct Tutorial {
      int activated_neurons_;
      int synapses_;
      int action_;
      bool first_attack_;
      bool first_damage_;
      bool action_active_;
      bool discovered_;
      bool epsp_target_set_;
      bool oxygen_;
      bool potassium_;
      bool chloride_;
      bool glutamate_;
      bool dopamine_;
      bool serotonin_;
    };
    Tutorial tutorial_;

    int lines_;
    int cols_;
    const std::string base_path_;
    bool pause_;
    Drawrer drawrer_;
    int status_;
    int mode_;

    // contexts 
    std::shared_mutex mutex_context_;  ///< mutex locked, when printing.
    std::map<int, Context> contexts_;
    int current_context_;
    std::vector<char> history_;

    // audio
    Audio audio_;
    AudioReceiver audio_data_;
    std::vector<std::string> audio_paths_; 
    std::string audio_file_path_;

    // settings
    bool stay_in_synapse_menu_;
    bool show_full_welcome_text_;
    bool music_on_;
    bool show_ai_tactics_;

    std::map<std::string, RankingEntry*> ranking_;

    // Selection methods
    
    /**
     * Selects one of x options. (Clears field and locks mutex; pauses game??)
     * @param[in] instruction to let the user know what to do
     * @param[in] force_selection indicating whether tp force user to select)
     * @param[in] mapping which maps a int to a string.
     * @param[in] splits indicating where to split options.
     * @param[in] error_msg shown in brackets after "selection not availible"
     */
    int SelectInteger(std::string instruction, bool force_selection, choice_mapping_t& mapping, 
        std::vector<size_t> splits, std::string error_msg);

    /**
     *  Stores infos for directory.
     */
    struct AudioSelector {
      std::string path_; ///< directory path
      std::string title_;  ///< title shown
      std::vector<std::pair<std::string, std::string>> options_;  ///< subdirs or files
    };
    static AudioSelector SetupAudioSelector(std::string path, std::string title, std::vector<std::string> paths);

    /**
     * Selects path to audio-file (Clears field).
     * @param[in] message to show
     * @return path to audio file.
     */
    std::string SelectAudio(std::string mdg);

    /**
     * Allows player to edit settings.
     */
    void EditSettings();

    /**
     * Loads and applies current settings.
     */
    void LoadSettings();

    /**
     * Loads local ranking list.
     */
    void LoadRanking();

    /**
     * Stores local ranking list.
     */
    void StoreRanking();

    /**
     * Prints ranking
     */
    void PrintRanking();

    // input methods

    /**
     * Input simple string (Clears field).
     * @param[in] instruction to let the user know what to do
     * @return user input (string)
     */
    std::string InputString(std::string msg);

    /**
     * Prints help (called on player-action).
     * Pauses game if single-player (or tutorial).
     * @param[in] data (unused)
     */
    void h_Help(std::shared_ptr<Data> data); 

    /**
     * Asks for confirmation then exits game. (called on player-action)
     * @param[in] data (unused)
     */
    void h_Quit(std::shared_ptr<Data> data); 

    /**
     * Simply refreshes screen.
     */
    void h_Refresh(std::shared_ptr<Data> data); 

    /**
     * Prints received message, then exits game (called on server-action)
     * @param[in] data
     */
    void h_Kill(std::shared_ptr<Data> data); 

    /**
     * Pauses/ unpauses game in single-player mode (called on player-action)
     * @param[in] data (unused)
     */
    void h_PauseOrUnpause(std::shared_ptr<Data> data);

    /**
     * Moves selection up in different contexts (called on player-action)
     * @param[in] data (unused)
     */
    void h_MoveSelectionUp(std::shared_ptr<Data> data);

    /**
     * Moves selection down in different contexts (called on player-action)
     * @param[in] data (unused)
     */
    void h_MoveSelectionDown(std::shared_ptr<Data> data);

    /**
     * Moves selection to left in different contexts (called on player-action)
     * @param[in] data (unused)
     */
    void h_MoveSelectionLeft(std::shared_ptr<Data> data);

    /**
     * Moves selection to right in different contexts (called on player-action)
     * @param[in] data (unused)
     */
    void h_MoveSelectionRight(std::shared_ptr<Data> data);

    /**
     * Selects game from lobby (called on player-action)
     * @param[in] data (unused)
     */
    void h_SelectGame(std::shared_ptr<Data> data); 

    /**
     * Changes between different view-points f.e. between resource- and
     * technology (called on player-action)
     * @param[in] data (unused)
     */
    void h_ChangeViewPoint(std::shared_ptr<Data> data);

    /**
     * Requests adding iron (called on player-action)
     * @param[in] data (unused)
     */
    void h_AddIron(std::shared_ptr<Data> data);

    /**
     * Requests removing iron (called on player-action)
     * @param[in] data (unused)
     */
    void h_RemoveIron(std::shared_ptr<Data> data);

    /**
     * Requests researching new technology (called on player-action)
     * @param[in] data (unused)
     */
    void h_AddTech(std::shared_ptr<Data> data); 

    /**
     * Displays technology as researched in drawrer (called on server-action)
     * @param[in] data 
     */
    void h_AddTechnology(std::shared_ptr<Data> data); 

    /** 
     * Requests building neuron (called on player- and server-action).
     * (Communication-thread till action finished)
     * @param[in] data 
     */
    void h_BuildNeuron(std::shared_ptr<Data> data);

    /** 
     * Requests building potential (called on player- and server-action).
     * (Communication-thread till action finished)
     * @param[in] data 
     */
    void h_BuildPotential(std::shared_ptr<Data> data);

    /**
     * Switches (back) to resource-context (called on player-action)
     * @param[in] data (unused)
     */
    void h_ToResourceContext(std::shared_ptr<Data> data);

    /**
     * Selects synapse (called on player- and server-action).
     * (Communication-thread till action finished)
     * @param[in] data 
     */
    void h_SendSelectSynapse(std::shared_ptr<Data> data);

    /**
     * Requests selecting waypoints for synapse (called on player- and server-action).
     * (Communication-thread till action finished)
     * @param[in] data 
     */
    void h_SetWPs(std::shared_ptr<Data> data);

    /**
     * Requests selecting target (epsp, ipsp or macro) for synapse (called on player- and server-action).
     * (Communication-thread till action finished)
     * @param[in] data 
     */
    void h_SetTarget(std::shared_ptr<Data> data);

    /**
     * Requests toggling swarm-attack for synapse (called on player-action).
     * @param[in] data 
     */
    void h_SwarmAttack(std::shared_ptr<Data> data);

    /**
     * Resets or quits synapse-context (called on player-action)
     * @param[in] data 
     */
    void h_ResetOrQuitSynapseContext(std::shared_ptr<Data> data);

    /**
     * Toggles between showing statistics-table or -graph (called on player-action)
     * @param[in] data (unused)
     */
    void h_ToggleGraphView(std::shared_ptr<Data> data);

    /**
     * Toggles showing resource (called on player-action). 
     * Takes resource from action-history
     * @param[in] data (unused)
     */
    void h_ToggleShowResource(std::shared_ptr<Data> data);


    // tutorial
    
    /**
     * Tutorial: reacts after oxygen is researched (called on server-action).
     * @param[in] data (unused)
     */
    void h_TutorialGetOxygen(std::shared_ptr<Data> data);

    /**
     * Tutorial: reacts after setting a unit (called on server-action).
     * @param[in] data 
     */
    void h_TutorialSetUnit(std::shared_ptr<Data> data);

    /**
     * Tutorial: reacts after neuron is built (called on server-action).
     * @param[in] data 
     */
    void h_TutorialBuildNeuron(std::shared_ptr<Data> data);
    
    /**
     * Tutorial: reacts after enemy neurons are scouted (called on server-action).
     * @param[in] data 
     */
    void h_TutorialScouted(std::shared_ptr<Data> data);

    /**
     * Tutorial: reacts when msg is set (called on server-action).
     * @param[in] data 
     */
    void h_TutorialSetMessage(std::shared_ptr<Data> data);

    /**
     * Tutorial: reacts on game-update (called on server-action).
     * @param[in] data 
     */
    void h_TutorialUpdateGame(std::shared_ptr<Data> data);

    /**
     * Gets next tutorial action if availible (called on player-action)
     * @param[in] data (unused)
     */
    void h_TutorialAction(std::shared_ptr<Data> data);

    /**
     * Initializes tutorial text (pause game, wait, set 'old' context and text,
     * switch to tutorial context, print (first) text).
     * @param[in] text 
     */
    void h_InitTutorialText(const texts::paragraphs_field_t& text);

    /**
     * Increases current text and call h_TextPrint (called on player-action)
     * @param[in] data (unused)
     */
    void h_TextNext(std::shared_ptr<Data> data);

    /**
     * Decreases current text and call h_TextPrint (called on player-action)
     * @param[in] data (unused)
     */
    void h_TextPrev(std::shared_ptr<Data> data);

    /**
     * Simply calls h_TextQuit() (called on player-action)
     * @param[in] data (unused)
     */
    void h_TextQuit(std::shared_ptr<Data> data);

    /**
     * Refreshes page showing "Analyzing audio..." (called on server-action)
     * @param[in] data (unused)
     */
    void m_Preparing(std::shared_ptr<Data> data);

    /**
     * Shows player main-menu with basic game infos and lets player pick mode or
     * changes settings (called on server-action).
     * @param[in] data (unused)
     */
    void m_SelectMode(std::shared_ptr<Data> data);

    /**
     * Selects audio for map and (main-)ai (called on server-action)
     * @param[in] data (unused)
     */
    void m_SelectAudio(std::shared_ptr<Data> data);

    /**
     * Adds audio-filename, sends audio if requested, adds ais for observer-game
     * (called on server-action)
     * @param[in] data 
     */
    void m_SendAudioInfo(std::shared_ptr<Data> data);

    /**
     * Stores audio-data received from server (called on server-action)
     * @param[in] data 
     */
    void m_GetAudioData(std::shared_ptr<Data> data);

    /**
     * Prints message centered on screen overwriting anything else (called on server-action).
     * @param[in] data 
     */
    void m_PrintMsg(std::shared_ptr<Data> data);

    /**
     * Initializes game on game-start (called on server-action)
     * @param[in] data 
     */
    void m_InitGame(std::shared_ptr<Data> data);

    /**
     * Updates game (called on server-action).
     * @param[in] data 
     */
    void m_UpdateGame(std::shared_ptr<Data> data);

    /**
     * Updates game-lobby (called on server-action).
     * @param[in] data 
     */
    void m_UpdateLobby(std::shared_ptr<Data> data);

    /**
     * Sets "small" message at the bottom of the screen (called on server-action)
     * @param[in] data 
     */
    void m_SetMsg(std::shared_ptr<Data> data);

    /**
     * Prints game-end-message, sets statictics then stops websocket-client (called on server-action)
     * @param[in] data 
     */
    void m_GameEnd(std::shared_ptr<Data> data);

    /** 
     * Adds new unit to field/ drawrer (called on server-action)
     * @param[in] data 
     */
    void m_SetUnit(std::shared_ptr<Data> data);

    /** 
     * Adds new units to field/ drawrer (called on server-action)
     * @param[in] data 
     */
    void m_SetUnits(std::shared_ptr<Data> data);

    /** 
     * Asks player (client) if they have song (called on server-action)
     * @param[in] data 
     */
    void m_SongExists(std::shared_ptr<Data> data);

    /**
     * Adds `pos`-field to current data, by taking pos from current field-position,
     * then calls action stored in context (called on player-action).
     * @param[in] data 
     */
    void h_AddPosition(std::shared_ptr<Data> data);

    /**
     * Add `start_pos`-field to current data, by taking pos from marker-position 
     * at last symbol, then calls action stored in context (called on player-action).
     * @param[in] data 
     */
    void h_AddStartPosition(std::shared_ptr<Data> data);

    // helper 
    
    /**
     * Switches to pick context.
     * @param[in] positions to pick from.
     * @param[in] msg to show (giving player hint what to do f.e. "select start-position")
     * @param[in] action to call upon selection.
     * @param[in] current data passed to called action
     * @param[in] current handlers to add to pick-context (f.e. quit-handler)
     */
    void SwitchToPickContext(std::vector<position_t> positions, std::string msg, std::string action, 
        std::shared_ptr<Data> data, std::vector<char> slip_handlers = {});

    /**
     * Switches to field context.
     * @param[in] pos to start selection from (usually nucleus)
     * @param[in] range of nucleus (highlighted green)
     * @param[in] action to call upon selection.
     * @param[in] msg to show (giving player hint what to do f.e. "select synapse position")
     * @param[in] current handlers to add to field-context (f.e. quit-handler)
     */
    void SwitchToFieldContext(position_t pos, int range, std::string action, std::shared_ptr<Data>data, 
        std::string msg, std::vector<char> slip_handlers = {});
    
    /**
     * Switches to resource context.
     * @param[in] msg to show (giving player hint what to do f.e. "select synapse position)
     */
    void SwitchToResourceContext(std::string msg = "");

    /**
     * Switches to synapse-context.
     * @param[in] current data to use in synapse-context.
     */
    void SwitchToSynapseContext(std::shared_ptr<Data> data = std::make_shared<Data>());

    /**
     * Either resets synapse-context, or goes back to resource-context, depending
     * on setting.
     * @param[in] synapse-context
     */
    void FinalSynapseContextAction(position_t synapse_pos);

    /**
     * Removes pick-context, if new_context is set changes to new context.
     * @param[in] new-context to move to (default=-1 -> don't move)
     */
    void RemovePickContext(int new_context=-1);

    /**
     * Changes back to last context.
     */
    void h_TextQuit();

    /**
     * Prints current text of text-context.
     */
    void h_TextPrint();

    /**
     * Pauses render and requests pausing game.
     */
    void Pause();

    /**
     * Unpauses render and requests unpausing game.
     */
    void UnPause();

    /**
     * Clears up ncurses and exits programm.
     */
    void WrapUp();

    /**
     * Sends song to server in multiple chunks. 
     * Informs user if file-size is to big.
     */
    void SendSong();
};

#endif
