#ifndef SRC_PLAYER_H_
#define SRC_PLAYER_H_

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "den.h"
#include "soldier.h"

#define COST_GOLD_GATHERER 23.43
#define COST_SILVER_GATHERER 4.84
#define COST_BRONZE_GATHERER 2.2

#define COST_SOLDIER 1.6

class Player {
  public:
    Player(std::pair<int, int> den_pos) : gold_(0), silver_(4), bronze_(2.4), gatherer_gold_(0), gatherer_silver_(0), gatherer_bronze_(0) {den_ = Den(); den_.cur_pos_ = den_pos; }


    // getter 
    const std::map<std::string, Soldier>& soldier() { return soldiers_; };

    // setter 

    // methods
    std::string get_status();

    void inc();

    std::string add_gatherer_bronze();

    std::string add_gatherer_silver();

    std::string add_gatherer_gold();

    std::string add_soldier(std::pair<int, int> pos);
    void update_soldiers();


  private: 
    float gold_;
    float silver_;
    float bronze_;
    int gatherer_gold_;
    int gatherer_silver_;
    int gatherer_bronze_;

    Den den_;
    std::map<std::string, Soldier> soldiers_;

};

#endif
