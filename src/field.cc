#include <cctype>
#include <curses.h>
#include <iostream>
#include <locale>
#include <string>
#include <utility>
#include <vector>
#include "field.h"
#include "player.h"

#define DEFAULT_COLORS 3
#define PLAYER 1
#define KI 2 

#define HILL ' ' 
#define DEN 'D' 
#define GOLD 'G' 
#define SILVER 'S' 
#define BRONZE 'B' 
#define FREE char(46)

int getrandom_int(int min, int max);

Field::Field(int lines, int cols) {
  lines_ = lines;
  cols_ = cols;
  std::cout << "Using lines: " << lines_ << std::endl;;
  std::cout << "Using cols:  " << cols_ << std::endl;

  // initialize empty field.
  for (int l=0; l<=lines_; l++) {
    field_.push_back({});
    for (int c=0; c<=cols_; c++)
      field_[l].push_back(char(46));
  }
}

std::pair<int, int> Field::add_player() {
  int y = getrandom_int(lines_ / 1.5, lines_-5);
  int x = getrandom_int(cols_ / 1.5, cols_-5);
  player_den_ = {y, x};
  player_[player_den_] = DEN;
  field_[y][x] = player_[player_den_];

  add_resources(y, x);
  return player_den_;
}

std::pair<int, int> Field::add_ki() {
  // Create random enemy den.
  int y = getrandom_int(5, lines_ / 3);
  int x = getrandom_int(5, cols_ / 3);
  ki_den_ = {y,x};
  ki_[ki_den_] = DEN;
  field_[y][x] = ki_[ki_den_];

  add_resources(y, x);
  return ki_den_;
}


void Field::add_resources(int y, int x) {
  auto pos = find_free(y, x, 2);
  field_[pos.first][pos.second] = GOLD;
  pos = find_free(y, x, 2);
  field_[pos.first][pos.second] = SILVER;
  pos = find_free(y, x, 2);
  field_[pos.first][pos.second] = BRONZE;
}

std::pair<int, int> Field::get_new_soldier_pos() {
  return find_free(player_den_.first, player_den_.second, 1);
}

void Field::add_hills() {
  // int type = getrandom_int(0, 2);
  
  for (int i=0; i<lines_; i++) {
    int start_y = getrandom_int(0, lines_);
    int start_x = getrandom_int(0, cols_);
    field_[start_y][start_x] = HILL;
    int type = getrandom_int(0, 4);

    // heap
    if (type == 1) {
      field_[start_y+1][start_x+1] = HILL;
      field_[start_y][start_x+1] = HILL;
      field_[start_y-1][start_x] = HILL;
      field_[start_y-1][start_x-1] = HILL;
    }
    
    // horizontel 
    else if (type == 2) {
      field_[get_line_in_range(start_y+1)][get_col_in_range(start_x)] = HILL;
      field_[get_line_in_range(start_y+2)][get_col_in_range(start_x)] = HILL;
      field_[get_line_in_range(start_y-1)][get_col_in_range(start_x)] = HILL;
      field_[get_line_in_range(start_y-2)][get_col_in_range(start_x)] = HILL;
    }
    
    // horizontel 
    else if (type == 3) {
      field_[get_line_in_range(start_y)][get_col_in_range(start_x-1)] = HILL;
      field_[get_line_in_range(start_y)][get_col_in_range(start_x-2)] = HILL;
      field_[get_line_in_range(start_y)][get_col_in_range(start_x+1)] = HILL;
      field_[get_line_in_range(start_y)][get_col_in_range(start_x+2)] = HILL;
    }
  }
}

void Field::update_field(Player *player, std::vector<std::vector<char>>& field) {
  for (auto it : player->soldier()) {
    int l = it.second.cur_pos_.first;
    int c = it.second.cur_pos_.second;
    if (field[l][c] == FREE)
      field[l][c] = '1';
    else {
      std::locale loc;
      char val = field[l][c];
      if (std::isdigit(val, loc)) {
        int num = val - '0';
        field[l][c] = num+1 + '0';
      }
    }
  }
}

void Field::print_field(Player* player, Player* ki) {
  auto field = field_;
  update_field(player, field);
  update_field(ki, field);

  for (int l=0; l<lines_; l++) {
    for (int c=0; c<cols_; c++) {
      if (ki_.count({l,c}) > 0)
        attron(COLOR_PAIR(PLAYER));
      else if (player_.count({l,c}) > 0)
        attron(COLOR_PAIR(KI));
      mvaddch(10+l, 10+2*c, field[l][c]);
      mvaddch(10+l, 10+2*c+1, ' ' );
      attron(COLOR_PAIR(DEFAULT_COLORS));
    }
  }
}

int Field::get_line_in_range(int l) {
  if (l < 0) 
    return 0;
  if (l > lines_)
    return lines_;
  return l;
}

int Field::get_col_in_range(int c) {
  if (c < 0) 
    return 0;
  if (c > cols_)
    return cols_;
  return c;
}

bool Field::in_field(int l, int c) {
  return (l >= 0 && l <= lines_ && c >= 0 && c <= cols_);
}

std::pair<int, int> Field::find_free(int l, int c, int t) {
  for (int i=0; i<lines_; i++) {
    if (in_field(l+t+i, c+t+i) && field_[l+t+i][c+t+i] == FREE)
      return {l+t+i, c+t+i};
    if (in_field(l-t-i, c+t+i) && field_[l-t-i][c+t+i] == FREE)
      return {l-t-i, c+t+i};
    if (in_field(l+t+i, c-t-i) && field_[l+t+i][c-t-i] == FREE)
      return {l+t+i, c-t-i};
    if (in_field(l-t-i, c-t-i) && field_[l-t-i][c-t-i] == FREE)
      return {l-t-i, c-t-i};
  }
  exit(400);
}


int getrandom_int(int min, int max) {
  int ran = min + (rand() % (max - min + 1)); 
  return ran;
}
