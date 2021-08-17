#ifndef SRC_GAME_H_
#define SRC_GAME_H_

#include <curses.h>
#include <mutex>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <shared_mutex>
#include <vector>

#include "field.h"
#include "player.h"
#include "units.h"

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
    Player* player_two_;
    bool game_over_;
    bool pause_;

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
    void DoActions();

    /**
     * Handls player input (Runs as thread).
     */
    void GetPlayerChoice();

    Position SelectPosition(Position start, int range);

    Position SelectBarack(Player* p);

    /**
     * Handles ki-towers and soldiers.
     */
    void HandleKi();

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

};

#endif
