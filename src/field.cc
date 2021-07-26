#include <cctype>
#include <curses.h>
#include <exception>
#include <iostream>
#include <locale>
#include <math.h>
#include <string>
#include <utility>
#include <vector>
#include "field.h"
#include "player.h"

#define DEFAULT_COLORS 3
#define PLAYER 1
#define KI 2 
#define RESOURCES 4 
#define IN_GRAPH 5 

#define HILL ' ' 
#define DEN 'D' 
#define GOLD 'G' 
#define SILVER 'S' 
#define BRONZE 'B' 
#define FREE char(46)

int getrandom_int(int min, int max);
int random_coordinate_shift(int x, int min, int max);

Field::Field(int lines, int cols) {
  lines_ = lines;
  cols_ = cols;
  printf("Using lines: %u, using cols: %u. \n", lines_, cols_);

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
  field_[y][x] = DEN;
  clear_field(y, x);

  add_resources(y, x);
  return player_den_;
}

std::pair<int, int> Field::add_ki() {
  // Create random enemy den.
  int y = getrandom_int(5, lines_ / 3);
  int x = getrandom_int(5, cols_ / 3);
  ki_den_ = {y,x};
  ki_[ki_den_] = DEN;
  field_[y][x] = DEN;
  clear_field(y, x);

  add_resources(y, x);
  return ki_den_;
}

void Field::clear_field(int l, int c) {
  field_[l][c+1] = FREE;
  field_[l+1][c] = FREE;
  field_[l][c-1] = FREE;
  field_[l-1][c] = FREE;
  field_[l+1][c+1] = FREE;
  field_[l+1][c-1] = FREE;
  field_[l-1][c+1] = FREE;
  field_[l-1][c-1] = FREE;
}

void Field::add_resources(int y, int x) {
  auto pos = find_free(y, x, 2, 4);
  field_[pos.first][pos.second] = GOLD;
  pos = find_free(y, x, 2, 4);
  field_[pos.first][pos.second] = SILVER;
  pos = find_free(y, x, 2, 4);
  field_[pos.first][pos.second] = BRONZE;
}

void Field::build_graph() {
  // Add all nodes.
  for (int l=0; l<lines_; l++) {
    for (int c=0; c<cols_; c++) {
      if (field_[l][c] != HILL)
        graph_.add_node(l, c);
    }
  }

  // For each node, add edges.
  for (auto node : graph_.nodes()) {
    // Node above 
    std::pair<int, int> pos = {node.second->line_-1, node.second->col_};
    if (in_field(pos.first, pos.second) && field_[pos.first][pos.second] != HILL && graph_.in_graph(pos))
      graph_.add_edge(node.second, graph_.nodes().at(pos));
    // Node below
    pos = {node.second->line_+1, node.second->col_};
    if (in_field(pos.first, pos.second) && field_[pos.first][pos.second] != HILL && graph_.in_graph(pos))
      graph_.add_edge(node.second, graph_.nodes().at(pos));
    // Node left
    pos = {node.second->line_, node.second->col_-1};
    if (in_field(pos.first, pos.second) && field_[pos.first][pos.second] != HILL && graph_.in_graph(pos))
      graph_.add_edge(node.second, graph_.nodes().at(pos));
    // Node right 
    pos = {node.second->line_, node.second->col_+1};
    if (in_field(pos.first, pos.second) && field_[pos.first][pos.second] != HILL && graph_.in_graph(pos))
      graph_.add_edge(node.second, graph_.nodes().at(pos));
    // Upper left
    pos = {node.second->line_-1, node.second->col_-1};
    if (in_field(pos.first, pos.second) && field_[pos.first][pos.second] != HILL && graph_.in_graph(pos))
      graph_.add_edge(node.second, graph_.nodes().at(pos));
    // lower left 
    pos = {node.second->line_+1, node.second->col_-1};
    if (in_field(pos.first, pos.second) && field_[pos.first][pos.second] != HILL && graph_.in_graph(pos))
      graph_.add_edge(node.second, graph_.nodes().at(pos));
    // Upper right
    pos = {node.second->line_-1, node.second->col_+1};
    if (in_field(pos.first, pos.second) && field_[pos.first][pos.second] != HILL && graph_.in_graph(pos))
      graph_.add_edge(node.second, graph_.nodes().at(pos));
    // lower right 
    pos = {node.second->line_+1, node.second->col_+1};
    if (in_field(pos.first, pos.second) && field_[pos.first][pos.second] != HILL && graph_.in_graph(pos))
      graph_.add_edge(node.second, graph_.nodes().at(pos));
  }

  // Remove all nodes not in main circle
  graph_.remove_invalid(player_den_);
  if (graph_.nodes().count(ki_den_) == 0)
    throw "Invalid wworld.";
}

std::pair<int, int> Field::get_new_soldier_pos() {
  return find_free(player_den_.first, player_den_.second, 1, 3);
}

void Field::add_hills() {
  // Generate lines*2 mountains.
  for (int i=0; i<lines_*2; i++) {
    // Generate random hill.
    int start_y = getrandom_int(0, lines_);
    int start_x = getrandom_int(0, cols_);
    field_[start_y][start_x] = HILL;

    // Generate random 5 hills around this hill.
    for (int j=1; j<=5; j++) {
      int y = get_x_in_range(random_coordinate_shift(start_y, 0, j), 0, lines_);
      int x = get_x_in_range(random_coordinate_shift(start_x, 0, j), 0, cols_);
      field_[y][x] = HILL;
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
      else if (field_[l][c] == 'G' || field_[l][c] == 'S' ||field_[l][c] == 'B')
        attron(COLOR_PAIR(RESOURCES));
      else if (graph_.in_graph({l, c})) 
        attron(COLOR_PAIR(IN_GRAPH));
      mvaddch(10+l, 10+2*c, field[l][c]);
      mvaddch(10+l, 10+2*c+1, ' ' );
      attron(COLOR_PAIR(DEFAULT_COLORS));
    }
  }
}

int Field::get_x_in_range(int x, int min, int max) {
  if (x < min) 
    return min;
  if (x > max)
    return max;
  return x;
}

bool Field::in_field(int l, int c) {
  return (l >= 0 && l <= lines_ && c >= 0 && c <= cols_);
}

std::pair<int, int> Field::find_free(int l, int c, int min, int max) {
  for (int attempts=0; attempts<100; attempts++) {
    for (int i=0; i<max; i++) {
      int y = get_x_in_range(random_coordinate_shift(l, min, min+i), 0, lines_);
      int x = get_x_in_range(random_coordinate_shift(c, min, min+i), 0, cols_);
      if (field_[y][x] == FREE)
        return {y, x};
    }
  }    
  exit(400);
}


int getrandom_int(int min, int max) {
  int ran = min + (rand() % (max - min + 1)); 
  return ran;
}

int random_coordinate_shift(int x, int min, int max) {
  // Determine decrease or increase of values.
  int plus_minus = getrandom_int(0, 1);
  int random_faktor = getrandom_int(min, max);
  return x + pow(-1, plus_minus)*random_faktor;
}
