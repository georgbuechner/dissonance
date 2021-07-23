#ifndef SRC_GAME_H_
#define SRC_GAME_H_

#include <string>
#include <vector>
#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
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

    void print_field();

    void do_actions();
    void get_player_choice();
};

#endif
