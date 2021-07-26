#include "player.h"
#include <mutex>
#include <shared_mutex>
#include <string>
#include <vector>

std::string create_id();

std::string Player::get_status() {
  return "gold: " + std::to_string(gold_)
    + ", silver: " + std::to_string(silver_)
    + ", bronze: " + std::to_string(bronze_)
    + ", gatherer_gold: " + std::to_string(gatherer_gold_)
    + ", gatherer_silver: " + std::to_string(gatherer_silver_)
    + ", gatherer_bronze: " + std::to_string(gatherer_bronze_) 
    + ", den-lp: " + std::to_string(den_.lp_)
    + ".";
}

void Player::inc() {
  gold_ += static_cast<double>(gatherer_gold_)/10;
  silver_ += static_cast<double>(gatherer_silver_)/5;
  bronze_ += static_cast<double>(gatherer_bronze_)/2;
}

std::string Player::add_gatherer_bronze() {
  std::unique_lock ul_bronze(mutex_bronze_);
  std::unique_lock ul_bronze_gatherer(mutex_bronze_gatherer_);
  if (bronze_ >= COST_BRONZE_GATHERER) {
    gatherer_bronze_ += 1;
    bronze_ -= COST_BRONZE_GATHERER;
    return "";
  }
  return "Add bronze-gatherer: not enough bronze (const: "+std::to_string(COST_BRONZE_GATHERER)+")";
}

std::string Player::add_gatherer_silver() {
  std::unique_lock ul_bronze(mutex_bronze_);
  std::unique_lock ul_solver_gatherer(mutex_bronze_gatherer_);
  if (bronze_ >= COST_SILVER_GATHERER) {
    gatherer_silver_ += 1;
    bronze_ -= COST_SILVER_GATHERER;
    return "";
  }
  return "Add silver-gatherer: not enough bronze (const: "+std::to_string(COST_SILVER_GATHERER)+")";
}

std::string Player::add_gatherer_gold() {
  std::unique_lock ul_bronze(mutex_bronze_);
  std::unique_lock ul_gold_gatherer(mutex_gold_gatherer_);

  if (bronze_ >= COST_GOLD_GATHERER) {
    gatherer_gold_ += 1;
    bronze_ -= COST_GOLD_GATHERER;
    return "";
  }
  return "Add gold-gatherer: not enough bronze (const: "+std::to_string(COST_GOLD_GATHERER)+")";
}

std::string Player::add_def() {
  std::unique_lock ul_bronze(mutex_bronze_);
  std::unique_lock ul_silver(mutex_silver_);
  if (bronze_ >= COST_DEF_BRONZE && silver_ >= COST_DEF_SILVER) {
    bronze_ -= COST_DEF_BRONZE;
    silver_ -= COST_DEF_SILVER;
    return "";
  }
  return "Add defence: not enough bronze or silber (const: "
    + std::to_string(COST_DEF_BRONZE )+ " bronce and " + std::to_string(COST_DEF_SILVER) + " silver)";
}


std::string Player::add_soldier(std::pair<int, int> pos, std::list<std::pair<int, int>> way) {
  std::unique_lock ul_silver(mutex_silver_);
  std::unique_lock ul_soldier(mutex_soldier_);
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
  std::shared_lock sl_soldier(mutex_soldier_);
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
  sl_soldier.unlock();
  std::unique_lock ul_soldier(mutex_soldier_);
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
