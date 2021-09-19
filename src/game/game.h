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
#include "game/field.h"
#include "player/audio_ki.h"
#include "player/player.h"
#include "objects/units.h"


class Game {
  public:
    /** 
     * Constructor initializing game with availible lines and columns.
     * @param[in] lines availible lines.
     * @param[in] cols availible cols
     */
    Game(int lines, int cols);

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
    Audio audio_;

    const int lines_;
    const int cols_;

    int difficulty_;

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

    Position SelectPosition(Position start, int range);

    Position SelectNeuron(Player* p, int type);

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

    void PrintCentered(Paragraphs paragraphs);
    void PrintCentered(int line, std::string txt);

    int SelectInteger(std::string msg, bool omit, std::vector<size_t>, 
        std::map<size_t, std::string> mapping={}, std::vector<size_t> splits = {});

    void ClearField();

};

#endif
