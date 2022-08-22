#ifndef SRC_CLIENT_CLIENT_GAME_H_
#define SRC_CLIENT_CLIENT_GAME_H_

#include "share/audio/audio.h"
#include "share/shemes/commands.h"
// #include "share/objects/dtos.h"
#include <memory>
#define NCURSES_NOMACROS
#include <cstddef>
#include <curses.h>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include "nlohmann/json_fwd.hpp"

#include "client/game/print/drawrer.h"
#include "share/tools/eventmanager.h"
#include "share/tools/context.h"
#include "share/defines.h"

class Client;

class ClientGame {
  public:

    /** 
     * Constructor initializing basic settings and ncurses.
     * @param[in] audio_base_path
     */
    ClientGame(std::string base_path, std::string username, bool muliplayer_availible);

    std::string base_path() {
      return base_path_;
    }

    void set_client(Client* ws_srv) {
      ws_srv_ = ws_srv;
    }

    void HandleAction(Command cmd);

    void GetAction();

    static void init();

  private: 
    // member variables.
    const std::string username_;
    const bool muliplayer_availible_;
    Client* ws_srv_;
    EventManager<std::string, ClientGame, std::shared_ptr<Data>> eventmanager_;
    EventManager<std::string, ClientGame, std::shared_ptr<Data>> eventmanager_tutorial_;
    std::string audio_data_;

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
    bool render_pause_;
    bool pause_;
    Drawrer drawrer_;
    int status_;
    int mode_;
    Audio audio_;
    Lobby lobby_;

    std::shared_mutex mutex_context_;  ///< mutex locked, when printing.
    std::map<int, Context> contexts_;
    int current_context_;
    std::vector<char> history_;

    std::vector<std::string> audio_paths_; 
    std::string audio_file_path_;

    // settings
    bool stay_in_synapse_menu_;
    bool show_full_welcome_text_;
    bool music_on_;
    bool show_ai_tactics_;


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

    struct AudioSelector {
      std::string path_;
      std::string title_;
      std::vector<std::pair<std::string, std::string>> options_;
    };

    AudioSelector SetupAudioSelector(std::string path, std::string title, std::vector<std::string> paths);

    /**
     * Select path to audio-file (Clears field).
     * @param[in] message to show
     * @return path to audio file.
     */
    std::string SelectAudio(std::string mdg);

    void EditSettings();
    nlohmann::json LoadSettingsJson();
    void LoadSettings();

    void ShowLobby();

    // input methods

    /**
     * Input simple string (Clears field)
     * @param[in] instruction to let the user know what to do
     * @return user input (string)
     */
    std::string InputString(std::string msg);

    void h_Help(std::shared_ptr<Data> data); ///< show help
    void h_Quit(std::shared_ptr<Data> data); ///< ingame: asks user whether to really quit.
    void h_Kill(std::shared_ptr<Data> data); ///< before game: kills game.
    void h_PauseAndUnPause(std::shared_ptr<Data> data);
    void h_MoveSelectionUp(std::shared_ptr<Data> data);
    void h_MoveSelectionDown(std::shared_ptr<Data> data);
    void h_MoveSelectionLeft(std::shared_ptr<Data> data);
    void h_MoveSelectionRight(std::shared_ptr<Data> data);
    void h_SelectGame(std::shared_ptr<Data> data); 
    void h_ChangeViewPoint(std::shared_ptr<Data> data);
    void h_AddIron(std::shared_ptr<Data> data);
    void h_RemoveIron(std::shared_ptr<Data> data);
    void h_AddTech(std::shared_ptr<Data> data); ///< Requests adding tech.
    void h_AddTechnology(std::shared_ptr<Data> data); ///< Response for successfully adding tech.

    void h_BuildNeuron(std::shared_ptr<Data> data);
    void h_BuildPotential(std::shared_ptr<Data> data);
    void h_ToResourceContext(std::shared_ptr<Data> data);

    void h_SendSelectSynapse(std::shared_ptr<Data> data);

    void h_SetWPs(std::shared_ptr<Data> data);
    void h_SetTarget(std::shared_ptr<Data> data);
    void h_SwarmAttack(std::shared_ptr<Data> data);
    void h_ResetOrQuitSynapseContext(std::shared_ptr<Data> data);

    void h_ToggleGraphView(std::shared_ptr<Data> data);
    void h_ToggleShowResource(std::shared_ptr<Data> data);

    // tutorial
    void h_TutorialGetOxygen(std::shared_ptr<Data> data);
    /**
     * Reacts after setting a unit. (Also resource activated/ inactivated)
     */
    void h_TutorialSetUnit(std::shared_ptr<Data> data);
    void h_TutorialBuildNeuron(std::shared_ptr<Data> data);
    void h_TutorialScouted(std::shared_ptr<Data> data);
    void h_TutorialSetMessage(std::shared_ptr<Data> data);
    void h_TutorialUpdateGame(std::shared_ptr<Data> data);
    void h_TutorialAction(std::shared_ptr<Data> data);

    void Pause();
    void UnPause();

    // text
    /**
     * Increases current text and call h_TextPrint
     * @param[in, out] msg (unchanged)
     */
    void h_TextNext(std::shared_ptr<Data> data);

    /**
     * Decreases current text and call h_TextPrint
     * @param[in, out] msg (unchanged)
     */
    void h_TextPrev(std::shared_ptr<Data> data);

    /**
     * Simply calls h_TextQuit()
     * @param[in, out] msg (unchanged)
     */
    void h_TextQuit(std::shared_ptr<Data> data);

    /**
     * Changes back to last context.
     */
    void h_TextQuit();

    /**
     * Prints current text of text-context.
     */
    void h_TextPrint();

    // command methods

    /**
     * Shows player main-menu: with basic game info and lets player pic singe/
     * muliplayer
    */
    void m_Preparing(std::shared_ptr<Data> data);
    void m_SelectMode(std::shared_ptr<Data> data);
    void m_SelectAudio(std::shared_ptr<Data> data);
    void m_SendAudioInfo(std::shared_ptr<Data> data);
    void m_GetAudioData(std::shared_ptr<Data> data);
    void m_PrintMsg(std::shared_ptr<Data> data);
    void m_InitGame(std::shared_ptr<Data> data);
    void m_UpdateGame(std::shared_ptr<Data> data);
    void m_UpdateLobby(std::shared_ptr<Data> data);
    void m_SetMsg(std::shared_ptr<Data> data);
    void m_GameEnd(std::shared_ptr<Data> data);
    void m_SetUnit(std::shared_ptr<Data> data);
    void m_SetUnits(std::shared_ptr<Data> data);

    void m_SongExists(std::shared_ptr<Data> data);

    /**
     * Add `pos`-field to current data, by taking pos from current field-position.
     */
    void h_AddPosition(std::shared_ptr<Data> data);

    /**
     * Add `start_pos`-field to current data, by taking pos from marker-position at last symbol.
     */
    void h_AddStartPosition(std::shared_ptr<Data> data);

    void SwitchToPickContext(std::vector<position_t> positions, std::string msg, std::string action, 
        std::shared_ptr<Data> data, std::vector<char> slip_handlers = {});
    void SwitchToFieldContext(position_t pos, int range, std::string action, std::shared_ptr<Data>data, 
        std::string msg, std::vector<char> slip_handlers = {});
    void SwitchToResourceContext(std::string msg = "");
    void SwitchToSynapseContext(std::shared_ptr<Data> data = std::make_shared<Data>());

    /**
     * Either resets synape-context, or goes back to resource-context, depending
     * on setting.
     */
    void FinalSynapseContextAction(position_t synapse_pos);
    void RemovePickContext(int new_context=-1);

    static std::map<int, std::map<char, void(ClientGame::*)(std::shared_ptr<Data> data)>> handlers_;

    void WrapUp();
    bool SendSong();
};

#endif
