#include <cmath>
#include <math.h>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <vector>

#include "codes.h"
#include "player.h"
#include "units.h"
#include "codes.h"
#include "utils.h"

#define HILL ' ' 
#define DEN 'D' 
#define GOLD 'G' 
#define SILVER 'S' 
#define BRONZE 'B' 
#define FREE char(46)
#define DEF 'T'

const std::map<int, Costs> initial_costs_ = {
  {Units::ACTIVATEDNEURON, {
      {Resources::OXYGEN, 8.9}, 
      {Resources::POTASSIUM, 0}, 
      {Resources::CHLORIDE, 0}, 
      {Resources::GLUTAMATE, 19.1}, 
      {Resources::DOPAMINE, 0}, 
      {Resources::SEROTONIN, 0}
    }
  },
  {Units::SYNAPSE, {
      {Resources::OXYGEN, 13.4}, 
      {Resources::POTASSIUM, 6.6}, 
      {Resources::CHLORIDE, 0}, 
      {Resources::GLUTAMATE, 0}, 
      {Resources::DOPAMINE, 0}, 
      {Resources::SEROTONIN, 0}
    }
  },
  {Units::EPSP, {
      {Resources::OXYGEN, 0}, 
      {Resources::POTASSIUM, 4.4}, 
      {Resources::CHLORIDE, 0}, 
      {Resources::GLUTAMATE, 0}, 
      {Resources::DOPAMINE, 0}, 
      {Resources::SEROTONIN, 0}
    }
  },
  {Units::IPSP, {
      {Resources::OXYGEN, 0}, 
      {Resources::POTASSIUM, 3.4}, 
      {Resources::CHLORIDE, 6.8}, 
      {Resources::GLUTAMATE, 0}, 
      {Resources::DOPAMINE, 0}, 
      {Resources::SEROTONIN, 0}
    }
  }
};

std::string create_id();

Player::Player(Position den_pos, int iron) : cur_range_(4), resource_curve_(3), 
    oxygen_boast_(0), total_oxygen_(5.5) {
  den_ = Nucleus(den_pos); 
  all_units_and_buildings_.insert(den_pos);
  resources_ = {
    { Resources::IRON, {iron, false}},
    { Resources::OXYGEN, {5.5, true}},
    { Resources::POTASSIUM, {0, false}},
    { Resources::CHLORIDE, {0, false}},
    { Resources::GLUTAMATE, {0, false}},
    { Resources::DOPAMINE, {0, false}},
    { Resources::SEROTONIN, {0, false}},
  };
}

std::string Player::GetCurrentStatusLineA() {
  std::scoped_lock scoped_lock(mutex_resources_);
  return "Iron " SYMBOL_IRON ": " + std::to_string(resources_.at(Resources::IRON).first)
    + ", oxygen boast: " + std::to_string(oxygen_boast_)
    + ", total oxygen " SYMBOL_OXYGEN ": " + std::to_string(total_oxygen_)
    + ", bound oxygen: " + std::to_string(bound_oxygen_)
    + ", oxygen: " + std::to_string(resources_.at(Resources::OXYGEN).first);
}

std::string Player::GetCurrentStatusLineB() {
  std::scoped_lock scoped_lock(mutex_resources_);
  return "potassium " SYMBOL_POTASSIUM ": " + std::to_string(resources_.at(Resources::POTASSIUM).first)
    + ", chloride " SYMBOL_CHLORIDE ": " + std::to_string(resources_.at(Resources::CHLORIDE).first)
    + ", glutamate " SYMBOL_GLUTAMATE ": " + std::to_string(resources_.at(Resources::GLUTAMATE).first)
    + ", dopamine " SYMBOL_DOPAMINE ": " + std::to_string(resources_.at(Resources::DOPAMINE).first)
    + ", serotonin " SYMBOL_SEROTONIN ": " + std::to_string(resources_.at(Resources::SEROTONIN).first);
}

std::string Player::GetCurrentStatusLineC() {
  std::shared_lock sl(mutex_den_);
  return "nucleus " SYMBOL_DEN " potential: " + std::to_string(den_.lp_) + "/" + std::to_string(den_.max_lp_);
}

// getter 
std::map<std::string, Epsp> Player::soldier() { 
  return soldiers_; 
};
std::map<Position, Synapse> Player::barracks() { 
  return barracks_; 
};
std::map<Position, ActivatedNeuron> Player::activated_neurons() { 
  return defence_towers_; 
}
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

int Player::resource_curve() {
  return resource_curve_;
}

int Player::iron() {
  std::shared_lock sl(mutex_resources_);
  return resources_.at(Resources::IRON).first;
}

std::map<int, Resource> Player::resources() {
   std::shared_lock sl(mutex_resources_);
  return resources_;
}

// setter 
void Player::set_resource_curve(int resource_curve) {
  std::unique_lock ul(mutex_resources_);
  resource_curve_ = resource_curve;
}

// methods 

void Player::AddBuilding(Position pos) {
  std::unique_lock ul(mutex_units_and_buildings_);
  all_units_and_buildings_.insert(pos);
}

bool Player::DamageSoldier(std::string id) {
  std::unique_lock ul(mutex_soldiers_);
  if (soldiers_.count(id) > 0) {
    soldiers_[id].attack_--;
    if (soldiers_[id].attack_ == 0)
      return true;
  }
  return false;
}

double faktor(int limit, double cur, int curve) {
  return (limit-cur)/(curve*limit);
}

void Player::IncreaseResources() {
  std::unique_lock ul_resources(mutex_resources_);
  for (auto& it : resources_) {
    int cur_oxygen = resources_[Resources::OXYGEN].first;
    if (it.first == Resources::IRON) 
      continue;
    // Add oxygen based on oxygen-boast and current oxygen.
    else if (it.first == Resources::OXYGEN) {
      it.second.first += oxygen_boast_* faktor(100, total_oxygen_, resource_curve_) * 1;
      total_oxygen_ = bound_oxygen_ + it.second.first;
    }
    // Add other resources based on current oxygen.
    else if (it.second.second)
      it.second.first += log(cur_oxygen+1) * faktor(70, it.second.first, resource_curve_) * 1;

    auto cur_time = std::chrono::steady_clock::now();
    if (utils::get_elapsed(last_iron_, cur_time) > 2500) {
      if (utils::getrandom_int(0, cur_oxygen*0.75) == 0)
        resources_[Resources::IRON].first++;
      last_iron_ = cur_time;
    }
  }
}

bool Player::DistributeIron(int resource) {
  std::unique_lock ul(mutex_resources_);
  if (resources_.count(resource) == 0) {
    return false;
  }
  else if (resource == Resources::OXYGEN) {
    oxygen_boast_++;
    resources_[Resources::IRON].first--;
  }
  else if (resources_[Resources::IRON].first < 2) {
    return false;
  }
  else if (resources_[resource].second) {
    return false;
  }
  else {
    resources_[resource].second = true;
    resources_[Resources::IRON].first-=2;
  }
  return true;
}

Costs Player::CheckResources(int unit) {
  std::shared_lock sl(mutex_resources_);
  // get costs for desired unit
  Costs needed = initial_costs_.at(unit);
  std::map<int, double> missing;

  for (const auto& it : needed) {
    if (resources_.at(it.first).first < it.second) 
      missing[it.first] = it.second - resources_.at(it.first).first;
  }
  return missing;
}

void Player::TakeResources(Costs costs) {
  std::unique_lock ul_resources(mutex_resources_);
  for (const auto& it : costs) {
    resources_[it.first].first -= it.second;
    if (it.first == Resources::OXYGEN)
      bound_oxygen_ += it.second;
  }
}

void Player::AddDefenceTower(Position pos) {
  // Take resources.
  TakeResources(initial_costs_.at(Units::ACTIVATEDNEURON));

  AddBuilding(pos);
  std::unique_lock ul_def(mutex_defence_towers_);
  defence_towers_[pos] = ActivatedNeuron(pos);
}

void Player::AddSoldier(Position pos, std::list<Position> way) {
  // Take resources.
  TakeResources(initial_costs_.at(Units::EPSP));

  // Add soldier
  std::unique_lock ul(mutex_soldiers_);
  soldiers_[create_id()] = Epsp(pos, way);
}

void Player::AddBarrack(Position pos) {
  // Take resources.
  TakeResources(initial_costs_.at(Units::SYNAPSE));

  // Add barrack
  AddBuilding(pos);
  std::unique_lock ul(mutex_soldiers_);
  barracks_[pos] = Synapse(pos);
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
        damage+=it.second.attack_;  // add potential to damage.
        soldiers_to_remove.push_back(it.first); // remove 
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
  for (auto& tower : defence_towers_) {
    // Check if tower's recharge is done.
    if (utils::get_elapsed(tower.second.last_action_, cur_time) > tower.second.speed_) {
      std::string dead_soldier = "";
      for (const auto& potential : enemy->soldier()) {
        int distance = utils::dist(tower.first, potential.second.pos_);
        if (distance < 3) {
          if (enemy->DamageSoldier(potential.first))
            dead_soldier = potential.first;
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

bool Player::IsActivatedResource(int resource) {
  return resources_.at(resource).second;
}

bool Player::DecreaseDenLp(int val) {
  std::unique_lock ul(mutex_den_);
  den_.lp_ += val;
  if (den_.lp_ >= den_.max_lp_)
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
