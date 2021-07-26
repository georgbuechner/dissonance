#ifndef SRC_PLAYER_H_
#define SRC_PLAYER_H_

#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <shared_mutex>
#include <vector>

#include "den.h"
#include "soldier.h"

#define COST_GOLD_GATHERER 23.43
#define COST_SILVER_GATHERER 4.84
#define COST_BRONZE_GATHERER 2.2
#define COST_DEF_BRONZE 5.2
#define COST_DEF_SILVER 3.2
#define COST_SOLDIER 1.6

class Player {
  public:
    Player(std::pair<int, int> den_pos, int silver=0) : gold_(0), silver_(silver), bronze_(2.4), gatherer_gold_(0), gatherer_silver_(0), gatherer_bronze_(0) {
      den_ = Den({den_pos, 2}); 
    }


    // getter 
    const std::map<std::string, Soldier> soldier() { return soldiers_; };
    std::pair<int, int> den_pos() { return std::make_pair(den_.cur_pos_.first, den_.cur_pos_.second); }

    // methods
    std::string get_status();

    void inc();

    std::string add_gatherer_bronze();

    std::string add_gatherer_silver();

    std::string add_gatherer_gold();

    std::string add_def();

    std::string add_soldier(std::pair<int, int> pos, std::list<std::pair<int, int>>);

    int update_soldiers(std::pair<int, int> enemy_den);

    void remove_soldier(std::string id) {
      if (soldiers_.count(id) > 0)
        soldiers_.erase(id);
    }

    bool is_player_soldier(std::pair<int, int> pos) {
      for (const auto& it : soldiers_) {
        if (it.second.cur_pos_ == pos)
          return true;
      }
      return false;
    }

    bool decrease_den_lp(int val) {
      den_.lp_ -= val;
      if (den_.lp_ <= 0)
        return true;
      return false;
    }

  private: 
    float gold_;
    float silver_;
    float bronze_;
    int gatherer_gold_;
    int gatherer_silver_;
    int gatherer_bronze_;

    std::shared_mutex mutex_gold_;
    std::shared_mutex mutex_silver_;
    std::shared_mutex mutex_bronze_;
    std::shared_mutex mutex_gold_gatherer_;
    std::shared_mutex mutex_silver_gatherer_;
    std::shared_mutex mutex_bronze_gatherer_;

    Den den_;
    std::map<std::string, Soldier> soldiers_;
    std::shared_mutex mutex_soldier_;

};

#endif
