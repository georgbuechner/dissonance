#ifndef SRC_FIELD_H_
#define SRC_FIELD_H_

#include "player.h"
#include <map>
#include <string>
#include <utility>
#include <vector>
#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define RED "\033[1;31m"
#define BLUE "\033[1;34m"
#define CLEAR "\033[1;0"

class Field {
  public:
    Field(int lines, int cols);

    int lines() { return lines_; }
    int cols() { return cols_; }

    void add_hills();
    std::pair<int, int> add_player();
    std::pair<int, int> add_ki();
    void add_resources(int l, int c);

    std::pair<int, int> get_new_soldier_pos();


    void update_field(Player* player_1, std::vector<std::vector<char>>& field);
    void print_field(Player* player, Player* ki);


  private: 
    int lines_;
    int cols_;
    std::vector<std::vector<char>> field_;
    std::map<std::pair<int, int>, char> player_;
    std::map<std::pair<int, int>, char> ki_;
    std::pair<int, int> player_den_;
    std::pair<int, int> ki_den_;

    int get_line_in_range(int l);
    int get_col_in_range(int c);
    bool in_field(int l, int c);
    std::pair<int, int> find_free(int l, int c, int t);
};


#endif
