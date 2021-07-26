#include "player.h"
#include <string>
#include <vector>

std::string create_id();

std::string Player::get_status() {
  return "gold: " + std::to_string(gold_)
    + ", silver: " + std::to_string(silver_)
    + ", bronze: " + std::to_string(bronze_)
    + ", gatherer_gold: " + std::to_string(gatherer_gold_)
    + ", gatherer_silver: " + std::to_string(gatherer_silver_)
    + ", gatherer_bronze: " + std::to_string(gatherer_bronze_) + ".";
}

void Player::inc() {
  gold_ += static_cast<double>(gatherer_gold_)/10;
  silver_ += static_cast<double>(gatherer_silver_)/5;
  bronze_ += static_cast<double>(gatherer_bronze_)/2;
}

std::string Player::add_gatherer_bronze() {
  if (bronze_ >= COST_BRONZE_GATHERER) {
    gatherer_bronze_ += 1;
    bronze_ -= COST_BRONZE_GATHERER;
    return "";
  }
  return "Add bronze-gatherer: not enough bronze (const: "+std::to_string(COST_BRONZE_GATHERER)+")";
}

std::string Player::add_gatherer_silver() {
  if (bronze_ >= COST_SILVER_GATHERER) {
    gatherer_silver_ += 1;
    bronze_ -= COST_SILVER_GATHERER;
    return "";
  }
  return "Add silver-gatherer: not enough bronze (const: "+std::to_string(COST_SILVER_GATHERER)+")";
}

std::string Player::add_gatherer_gold() {
  if (bronze_ >= COST_GOLD_GATHERER) {
    gatherer_gold_ += 1;
    bronze_ -= COST_GOLD_GATHERER;
    return "";
  }
  return "Add gold-gatherer: not enough bronze (const: "+std::to_string(COST_GOLD_GATHERER)+")";
}

std::string Player::add_soldier(std::pair<int, int> pos, std::list<std::pair<int, int>> way) {
  if (silver_ >= COST_SOLDIER) {
    silver_ -= COST_SOLDIER;
    std::string id = create_id();
    soldiers_[id] = Soldier();
    soldiers_[id].cur_pos_ = pos;
    soldiers_[id].way_ = way;
    return "";
  }
  return "Add soldier: not enough silver (const: "+std::to_string(COST_SOLDIER)+")";
}

int Player::update_soldiers(std::pair<int, int> enemy_den) {
  std::vector<std::string> soldiers_to_remove;
  int damage = 0;
  for (auto& it : soldiers_) {
    if (it.second.way_.size() > 0) {
      it.second.cur_pos_ = it.second.way_.front(); 
      it.second.way_.pop_front();
      if (it.second.cur_pos_ == enemy_den) {
        damage++; 
        soldiers_to_remove.push_back(it.first);
      }
    }
  }
  for (const auto& it : soldiers_to_remove) 
    soldiers_.erase(it);
  return damage;
}

std::string create_id() {
  std::string id = "";
  for (int i=0; i<10; i++) {
    int ran = rand() % 9;
    id += std::to_string(ran);
  }
  return id;
}
