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

class Game {
  public:
    Game(int lines, int cols);

    void play();

  private: 
    Field* field_;
    Player* player_;
    Player* ki_;

    std::shared_mutex mutex_print_field_;

    bool game_over_;

    void print_field();

    void do_actions();
    void get_player_choice();
};

#endif
