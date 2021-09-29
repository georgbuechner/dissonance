#ifndef SRC_GAME_FILE_BROWSER_
#define SRC_GAME_FILE_BROWSER_ 

#include <cstddef>
#include <curses.h>
#include <mutex>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <shared_mutex>
#include <vector>

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
}; 

#endif
