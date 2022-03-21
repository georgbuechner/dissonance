#ifndef SRC_CLIENT_CLIENT_GAME_H_
#define SRC_CLIENT_CLIENT_GAME_H_

#include "share/audio/audio.h"
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

    void set_audio_file_path(std::string path) {
      audio_file_path_ = path;
    }

    void set_client(Client* ws_srv) {
      ws_srv_ = ws_srv;
    }

    nlohmann::json HandleAction(nlohmann::json);

    void GetAction();

    static void init();

  private: 
    // member variables.
    const std::string username_;
    const bool muliplayer_availible_;
    Client* ws_srv_;
    EventManager<std::string, ClientGame, nlohmann::json&> eventmanager_;
    int lines_;
    int cols_;
    const std::string base_path_;
    bool render_pause_;
    Drawrer drawrer_;
    int status_;
    int mode_;
    Audio audio_;

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

    // input methods

    /**
     * Input simple string (Clears field)
     * @param[in] instruction to let the user know what to do
     * @return user input (string)
     */
    std::string InputString(std::string msg);

    void h_Quit(nlohmann::json&); // ingame: asks user whether to really quit.
    void h_Kill(nlohmann::json&); // before game: kills game.
    void h_MoveSelectionUp(nlohmann::json&);
    void h_MoveSelectionDown(nlohmann::json&);
    void h_MoveSelectionLeft(nlohmann::json&);
    void h_MoveSelectionRight(nlohmann::json&);
    void h_ChangeViewPoint(nlohmann::json&);
    void h_AddIron(nlohmann::json&);
    void h_RemoveIron(nlohmann::json&);
    void h_AddTech(nlohmann::json&);

    void h_BuildNeuron(nlohmann::json&);
    void h_BuildPotential(nlohmann::json&);

    void h_SendSelectSynapse(nlohmann::json&);

    void h_SetWPs(nlohmann::json&);
    void h_EpspTarget(nlohmann::json&);
    void h_IpspTarget(nlohmann::json&);
    void h_SwarmAttack(nlohmann::json&);
    void h_ResetOrQuitSynapseContext(nlohmann::json&);

    // command methods

    /**
     * Shows player main-menu: with basic game info and lets player pic singe/
     * muliplayer
    */
    void m_SelectMode(nlohmann::json&);
    void m_SelectAudio(nlohmann::json&);
    void m_PrintMsg(nlohmann::json&);
    void m_InitGame(nlohmann::json&);
    void m_UpdateGame(nlohmann::json&);
    void m_SetMsg(nlohmann::json&);
    void m_GameEnd(nlohmann::json&);
    void m_SetUnit(nlohmann::json&);
    void m_SetUnits(nlohmann::json&);

    /**
     * Add `pos`-field to current data, by taking pos from current field-position.
     */
    void h_AddPosition(nlohmann::json&);

    /**
     * Add `start_pos`-field to current data, by taking pos from marker-position at last symbol.
     */
    void h_AddStartPosition(nlohmann::json&);

    void SwitchToPickContext(std::vector<position_t> positions, std::string msg, std::string action, 
        nlohmann::json data, std::vector<char> slip_handlers = {});
    void SwitchToFieldContext(position_t pos, int range, std::string action, nlohmann::json data, std::string msg, 
        std::vector<char> slip_handlers = {});
    void SwitchToResourceContext(std::string msg = "");
    void SwitchToSynapseContext(nlohmann::json data = nlohmann::json());
    /**
     * Either resets synape-context, or goes back to resource-context, depending
     * on setting.
     */
    void FinalSynapseContextAction(position_t synapse_pos);
    void RemovePickContext(int new_context=-1);

    static std::map<int, std::map<char, void(ClientGame::*)(nlohmann::json&)>> handlers_;

    void WrapUp();
};

#endif
