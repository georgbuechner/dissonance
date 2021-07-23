#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "game.h"



#define ITERMAX 10000

int main(void) {
  
  srand (time(NULL));

  // initialize curses
  initscr();
  cbreak();
  noecho();

  clear();

  // init colors
  use_default_colors();
  start_color();
  init_pair(1, COLOR_BLUE, -1);
  init_pair(2, COLOR_RED, -1);
  init_pair(3, -1, -1);
  init_pair(4, COLOR_CYAN, -1);

  // initialize game.
  Game game((LINES-20), (COLS-20) / 2);

  // start game
  game.play();
  
  refresh();

  endwin();

  exit(0);
}
