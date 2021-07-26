#ifndef SRC_FIELD_H_
#define SRC_FIELD_H_

#include "player.h"
#include <map>
#include <mutex>
#include <string>
#include <shared_mutex>
#include <utility>
#include <vector>
#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "graph.h"

class Field {
  public:
    Field(int lines, int cols);

    int lines() { return lines_; }
    int cols() { return cols_; }

    void add_hills();
    std::pair<int, int> add_player();
    std::pair<int, int> add_ki();
    void add_resources(int l, int c);
    void build_graph();

    std::pair<int, int> get_new_soldier_pos(bool player);
    void get_new_defence_pos(bool player);

    void update_field(Player* player_1, std::vector<std::vector<char>>& field);
    void handle_def(Player* player, Player* ki);
    void print_field(Player* player, Player* ki, bool show_in_graph=false);

    std::list<std::pair<int, int>> get_way_for_soldier(std::pair<int, int> pos, bool player) {
      if (player)
        return graph_.find_way(pos, ki_den_);
      else
        return graph_.find_way(pos, player_den_);
    }


  private: 
    int lines_;
    int cols_;
    std::vector<std::vector<char>> field_;
    Graph graph_;
    std::map<std::pair<int, int>, char> player_units_;
    std::map<std::pair<int, int>, char> ki_units_;
    std::pair<int, int> player_den_;
    std::pair<int, int> ki_den_;

    std::shared_mutex mutex_field_;
    std::shared_mutex mutex_player_units_;
    std::shared_mutex mutex_ki_units_;

    /**
     * Gets x in range, returning 0 if x<min, max ist x > max, and x otherwise.
     * @param x position to check
     * @param max maximum allowed
     * @param min minimum allowed
     * @return 0 if x<min, max ist x > max and x otherwise.
     */
    int get_x_in_range(int x, int min, int max);
    bool in_field(int l, int c);
    
    /** 
     * Finds the next free position near a given position with min and max
     * deviation.
     * @param l initial line
     * @param c initial column.
     * @param min distance 
     * @param max distance
     * @return free position in given range.
     */
    std::pair<int, int> find_free(int l, int c, int min, int max);

    void clear_field(int l, int c);
};


#endif
