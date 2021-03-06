#ifndef SRC_GAME_H_
#define SRC_GAME_H_

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

class Game {
  public:
    /** 
     * Constructor initializing game with availible lines and columns.
     * @param[in] lines availible lines.
     * @param[in] cols availible cols
     */
    Game(int lines, int cols, int left_border, std::string audio_base_path);

    /**
     * Starts game.
     */
    void play();

  private: 
    Field* field_;
    Player* player_one_;
    AudioKi* player_two_;
    bool game_over_;
    bool pause_;
    bool resigned_;
    Audio audio_;
    const std::string base_path_;
    std::vector<std::string> audio_paths_;

    const int lines_;
    const int cols_;
    const int left_border_;

    int difficulty_;

    std::vector<int> played_levels_;

    std::shared_mutex mutex_print_field_;  ///< mutex locked, when printing field.

    /**
     * Constantly checks whether to do actions (Runs as thread).
     * Actions might be:
     * - increase player resources
     * - move soldiers (player and ki)
     * - handle defence actions (remove soldiers when hit by tower).
     * - ki: add new soldier
     * - ki: add defence tower
     * - ki: lower time to add new soldier/ defence tower.
     */
    void RenderField();

    /**
     * Handls player input (Runs as thread).
     */
    void GetPlayerChoice();

    position_t SelectPosition(position_t start, int range);

    position_t SelectNeuron(Player* p, int type);
    position_t SelectFieldPositionByAlpha(std::vector<position_t> positions, std::string msg);

    void DistributeIron();

    /**
     * Handles ki-towers and soldiers.
     */
    void HandleActions();

    /**
     * Print help line, field and status player's status line.
     */
    void PrintFieldAndStatus();

    /**
     * Prints a message for the user below the status line.
     * @param[in] msg message to show.
     * @param[in] error indicates, whether msg shall be printed red.
     */
    void PrintMessage(std::string message, bool error);

    /**
     * Sets game over and prints message.
     * @param msg victory/ failure message.
     */
    void SetGameOver(std::string msg);

    void PrintCentered(texts::paragraphs_t paragraphs);
    void PrintCentered(int line, std::string txt);
    void PrintCenteredColored(int line, std::vector<std::pair<std::string, int>> txt_with_color);

    void PrintHelpLine();

    int SelectInteger(std::string msg, bool omit, choice_mapping_t& mapping, std::vector<size_t> splits);

    std::string SelectAudio();

    struct AudioSelector {
      std::string path_;
      std::string title_;
      std::vector<std::pair<std::string, std::string>> options_;
    };

    AudioSelector SetupAudioSelector(std::string path, std::string title, std::vector<std::string> paths);

    void ClearField();

    std::string InputString(std::string msg);

    position_t SelectNucleus(Player*);

};

#endif
