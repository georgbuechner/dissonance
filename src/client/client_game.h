#ifndef SRC_CLIENT_CLIENT_GAME_H_
#define SRC_CLIENT_CLIENT_GAME_H_

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

class ClientGame {
  public:

    /** 
     * Constructor initializing basic settings and ncurses.
     * @param[in] relative_size
     * @param[in] audio_base_path
     */
    ClientGame(bool relative_size, std::string base_path);

    /**
     * Shows player main-menu: with basic game info and lets player pic singe/
     * muliplayer
     */
    void Welcome();

  private: 
    // member variables.
    int lines_;
    int cols_;
    int left_border_;
    const std::string base_path_;
    std::shared_mutex mutex_print_;  ///< mutex locked, when printing.

    std::vector<std::string> audio_paths_; 

    // print methods
      
    /**
     * Clears and refreshes field, locks mutex.
     */
    void ClearField();

    /**
     * Prints a single line centered. (Does not clear screen, does not lock mutex)
     * @param[in] l (what line to print to)
     * @param[in] line (text to print
     */
    void PrintCenteredLine(int l, std::string line);

    /**
     * Prints a paragraph (mulitple lines of text) to the center of the screen.
     * (Does not clear screen, does not lock mutex)
     * @param[in] paragraph
     */
    void PrintCenteredParagraph(texts::paragraph_t paragraph);

    /**
     * Prints mulitple paragraphs to the center of the screen, clearing screen
     * before every new paragraph and locking mutex.
     * @param[in] paragraphs
     */
    void PrintCenteredParagraphs(texts::paragraphs_t paragraph);
   

   
};

#endif
