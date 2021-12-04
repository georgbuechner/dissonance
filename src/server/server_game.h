#ifndef SRC_SERVER_GAME_H_
#define SRC_SERVER_GAME_H_

#define NCURSES_NOMACROS
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

class ServerGame {
  public:
    /** 
     * Constructor initializing game with availible lines and columns.
     * @param[in] lines availible lines.
     * @param[in] cols availible cols
     */
    ServerGame(int lines, int cols, int mode, std::string base_path);

    /**
     * Handls input
     */
    nlohmann::json HandleInput(std::string command, std::string player, nlohmann::json data);

  private: 
    Field* field_;
    Player* player_one_;
    Player* player_two_;
    bool game_over_;
    bool pause_;
    bool resigned_;
    Audio audio_;

    const int mode_;
    const int lines_;
    const int cols_;

    // handlers
    nlohmann::json InitializeGame(nlohmann::json data);

    nlohmann::json DistributeIron(Player* player, std::pair<bool, std::string> error = {false, ""});
};

#endif
