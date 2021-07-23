#ifndef SRC_PLAYER_H_
#define SRC_PLAYER_H_

#include <iostream>
#include <string>
#include <vector>

#define COST_GOLD_GATHERER 23.43
#define COST_SILVER_GATHERER 4.84
#define COST_BRONZE_GATHERER 2.2

#define COST_SOLDIER 1.6

class Player {
  public:
    Player() : gold_(0), silver_(0), bronze_(2.4), gatherer_gold_(0), gatherer_silver_(0), gatherer_bronze_(0) {}

    std::string get_status() {
      return "gold: " + std::to_string(gold_)
        + ", silver: " + std::to_string(silver_)
        + ", bronze: " + std::to_string(bronze_)
        + ", gatherer_gold: " + std::to_string(gatherer_gold_)
        + ", gatherer_silver: " + std::to_string(gatherer_silver_)
        + ", gatherer_bronze: " + std::to_string(gatherer_bronze_) + ".";
    }

    void inc() {
      gold_ += static_cast<double>(gatherer_gold_)/10;
      silver_ += static_cast<double>(gatherer_silver_)/5;
      bronze_ += static_cast<double>(gatherer_bronze_)/2;
    }

    std::string add_gatherer_bronze() {
      if (bronze_ >= COST_BRONZE_GATHERER) {
        gatherer_bronze_ += 1;
        bronze_ -= COST_BRONZE_GATHERER;
        return "";
      }
      return "Add bronze-gatherer: not enough bronze (const: "+std::to_string(COST_BRONZE_GATHERER)+")";
    }

    std::string add_gatherer_silver() {
      if (bronze_ >= COST_SILVER_GATHERER) {
        gatherer_silver_ += 1;
        bronze_ -= COST_SILVER_GATHERER;
        return "";
      }
      return "Add silver-gatherer: not enough bronze (const: "+std::to_string(COST_SILVER_GATHERER)+")";
    }

    std::string add_gatherer_gold() {
      if (bronze_ >= COST_GOLD_GATHERER) {
        gatherer_gold_ += 1;
        bronze_ -= COST_GOLD_GATHERER;
        return "";
      }
      return "Add gold-gatherer: not enough bronze (const: "+std::to_string(COST_GOLD_GATHERER)+")";
    }

    std::string add_soldier() {
      if (silver_ >= COST_SOLDIER) {
        silver_ -= COST_SOLDIER;
        return "";
      }
      return "Add soldier: not enough silver (const: "+std::to_string(COST_SOLDIER)+")";
    }


  private: 
    float gold_;
    float silver_;
    float bronze_;
    int gatherer_gold_;
    int gatherer_silver_;
    int gatherer_bronze_;
};

#endif
