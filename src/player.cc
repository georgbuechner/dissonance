#include <mutex>
#include <shared_mutex>
#include <string>
#include <vector>

#include "codes.h"
#include "player.h"
#include "units.h"
#include "utils.h"

#define HILL ' ' 
#define DEN 'D' 
#define GOLD 'G' 
#define SILVER 'S' 
#define BRONZE 'B' 
#define FREE char(46)
#define DEF 'T'

const std::map<int, Costs> initial_costs_ = {
  {Units::GOLD_GATHERER, Costs{0, 0, 23.43, 1}},
  {Units::SILVER_GATHERER, Costs{0, 0, 4.84, 1}}, 
  {Units::BRONZE_GATHERER, Costs{0, 0, 2.2, 1}}, 
  {Units::ATK, Costs{0, 1.6, 0, 1}},
  {Units::BARACKS, Costs{0, 2.6, 10, 1}},
  {Units::DEFENCE, Costs{0, 4.2, 5.2, 1}}, 
};

std::string create_id();

std::string Player::GetCurrentStatusLine() {
  std::scoped_lock scoped_lock(mutex_resources_, mutex_gatherer_);
  return "gold: " + std::to_string(gold_)
    + ", silver: " + std::to_string(silver_)
    + ", bronze: " + std::to_string(bronze_)
    + ", gatherer_gold: " + std::to_string(gatherer_gold_)
    + ", gatherer_silver: " + std::to_string(gatherer_silver_)
    + ", gatherer_bronze: " + std::to_string(gatherer_bronze_) 
    + ", den-lp: " + std::to_string(den_.lp_)
    + ".";
}

// getter 
std::map<std::string, Soldier> Player::soldier() { 
  return soldiers_; 
};
std::map<Position, Barrack> Player::barracks() { 
  return barracks_; 
};

std::set<Position> Player::units_and_buildings() {
  return all_units_and_buildings_;
}

Position Player::den_pos() { 
  std::shared_lock sl(mutex_den_); 
  return den_.pos_;
}

int Player::cur_range() { 
  return cur_range_;
}

// methods 

void Player::AddBuilding(Position pos) {
  std::unique_lock ul(mutex_units_and_buildings_);
  all_units_and_buildings_.insert(pos);
}

void Player::IncreaseResources() {
  std::unique_lock ul_resources(mutex_resources_);
  gold_ += static_cast<double>(gatherer_gold_)/10;
  silver_ += static_cast<double>(gatherer_silver_)/5;
  bronze_ += static_cast<double>(gatherer_bronze_)/2;
}

Costs Player::CheckResources(int unit) {
  std::shared_lock sl(mutex_resources_);
  Costs needed = initial_costs_.at(unit);
  Costs missing = Costs({0,0,0,0});
  if (gold_ < needed.gold_)
    missing.gold_ = needed.gold_ - gold_;
  if (silver_< needed.silver_)
    missing.silver_ = needed.silver_ - silver_;
  if (bronze_ < needed.bronze_)
    missing.bronze_ = needed.bronze_ - bronze_;
  return missing;
}

void Player::TakeResources(Costs costs) {
  std::unique_lock ul_resources(mutex_resources_);
  bronze_ -= costs.bronze_;
  silver_ -= costs.silver_;
  gold_ -= costs.gold_;
}

void Player::AddGatherer(int unit) {
  // Take resources.
  TakeResources(initial_costs_.at(unit));

  // Add depeding on type of gatherer.
  std::unique_lock ul_gatherer(mutex_gatherer_);
  if (unit == Units::BRONZE_GATHERER)
    gatherer_bronze_++;
  else if (unit == Units::SILVER_GATHERER)
    gatherer_silver_++;
  else if (unit == Units::GOLD_GATHERER)
    gatherer_gold_++;
}

void Player::AddDefenceTower(Position pos) {
  // Take resources.
  TakeResources(initial_costs_.at(Units::DEFENCE));

  AddBuilding(pos);
  std::unique_lock ul_def(mutex_defence_towers_);
  defence_towers_[pos] = DefenceTower(pos);
}

void Player::AddSoldier(Position pos, std::list<Position> way) {
  // Take resources.
  TakeResources(initial_costs_.at(Units::ATK));

  // Add soldier
  std::unique_lock ul(mutex_soldiers_);
  soldiers_[create_id()] = Soldier(pos, way);
}

void Player::AddBarrack(Position pos) {
  // Take resources.
  TakeResources(initial_costs_.at(Units::BARACKS));

  // Add barrack
  AddBuilding(pos);
  std::unique_lock ul(mutex_soldiers_);
  barracks_[pos] = Barrack(pos);
}

int Player::MoveSoldiers(Position enemy_den) {
  // Move soldiers along the way to it's target and check if target is reached.
  std::shared_lock sl_soldier(mutex_soldiers_);
  std::vector<std::string> soldiers_to_remove;
  auto cur_time = std::chrono::steady_clock::now();
  int damage = 0;
  for (auto& it : soldiers_) {
    // Only move soldier, if way not empty and recharge is done.
    if (it.second.way_.size() > 0 && utils::get_elapsed(it.second.last_action_, cur_time) > it.second.speed_) {
      it.second.pos_ = it.second.way_.front(); 
      it.second.way_.pop_front();
      if (it.second.pos_ == enemy_den) {
        damage++; 
        soldiers_to_remove.push_back(it.first);
      }
      it.second.last_action_ = cur_time;  // soldier did action, so update last_action_.
    }
  }
  sl_soldier.unlock();
  // Remove soldiers which have reached it's target.
  std::unique_lock ul_soldier(mutex_soldiers_);
  for (const auto& it : soldiers_to_remove) 
    soldiers_.erase(it);
  return damage;
}

void Player::RemoveSoldier(std::string id) {
  std::unique_lock ul(mutex_soldiers_);
  if (soldiers_.count(id) > 0)
    soldiers_.erase(id);
}

void Player::HandleDef(Player* enemy) {
  std::shared_lock sl(mutex_units_and_buildings_);
  auto cur_time = std::chrono::steady_clock::now();
  for (auto tower : defence_towers_) {
    // Check if tower's recharge is done.
    if (utils::get_elapsed(tower.second.last_action_, cur_time) > tower.second.speed_) {
      std::string dead_soldier = "";
      for (auto soldier : enemy->soldier()) {
        int distance = utils::dist(tower.first, soldier.second.pos_);
        if (distance < 3) {
          dead_soldier = soldier.first;
          tower.second.last_action_ = cur_time;  // tower did action, so update last_action_.
          break;  // break loop, since every tower can destroy only one soldier.
        }
      }
      enemy->RemoveSoldier(dead_soldier);
    }
  }
}

bool Player::IsSoldier(Position pos) {
  std::shared_lock sl(mutex_soldiers_);
  for (const auto& it : soldiers_) {
    if (it.second.pos_ == pos)
      return true;
  }
  return false;
}

bool Player::DecreaseDenLp(int val) {
  std::unique_lock ul(mutex_den_);
  den_.lp_ -= val;
  if (den_.lp_ <= 0)
    return true;
  return false;
}


std::string create_id() {
  std::string id = "";
  for (int i=0; i<10; i++) {
    int ran = rand() % 9;
    id += std::to_string(ran);
  }
  return id;
}
