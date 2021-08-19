#include <cmath>
#include <cwchar>
#include <math.h>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <vector>

#include "codes.h"
#include "curses.h"
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

Player::Player(Position nucleus_pos, int iron) : cur_range_(4), resource_curve_(3), 
    oxygen_boast_(0), total_oxygen_(5.5) {
  nucleus_ = Nucleus(nucleus_pos); 
  all_neurons_.insert(nucleus_pos);
  resources_ = {
    { Resources::IRON, {iron, false}},
    { Resources::OXYGEN, {5.5, true}},
    { Resources::POTASSIUM, {0, false}},
    { Resources::CHLORIDE, {0, false}},
    { Resources::GLUTAMATE, {0, false}},
    { Resources::DOPAMINE, {0, false}},
    { Resources::SEROTONIN, {0, false}},
  };
  technologies_ = {
    {Technology::WAY, {0,3}},
    {Technology::SWARM, {0,3}},
    {Technology::TARGET, {0,3}},
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
  std::shared_lock sl(mutex_nucleus_);
  return "nucleus " SYMBOL_DEN " potential: " + std::to_string(nucleus_.lp_) + "/" + std::to_string(nucleus_.max_lp_);
}

// getter 
std::map<std::string, Epsp> Player::epsps() { 
  std::shared_lock sl(mutex_potentials_);
  return epsps_; 
}
std::map<std::string, Ipsp> Player::ipsps() { 
  std::shared_lock sl(mutex_potentials_);
  return ipsps_; 
}
std::map<Position, Synapse> Player::synapses() { 
  std::shared_lock sl(mutex_all_neurons_);
  return synapses_; 
}

std::map<Position, ActivatedNeuron> Player::activated_neurons() { 
  std::shared_lock sl(mutex_all_neurons_);
  return activated_neurons_; 
}

std::set<Position> Player::neurons() {
  std::shared_lock sl(mutex_all_neurons_);
  return all_neurons_;
}

Position Player::nucleus_pos() { 
  std::shared_lock sl(mutex_nucleus_); 
  return nucleus_.pos_;
}
int Player::nucleus_potential() { 
  std::shared_lock sl(mutex_nucleus_); 
  return nucleus_.lp_;
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

std::map<int, TechXOf> Player::technologies() {
  std::shared_lock sl(mutex_technologies_);
  return technologies_;
}

// setter 
void Player::set_resource_curve(int resource_curve) {
  std::unique_lock ul(mutex_resources_);
  resource_curve_ = resource_curve;
}

// methods 

double Player::Faktor(int limit, double cur) {
  return (limit-cur)/(resource_curve_*limit);
}

void Player::IncreaseResources() {
  std::unique_lock ul_resources(mutex_resources_);
  for (auto& it : resources_) {
    int cur_oxygen = resources_[Resources::OXYGEN].first;
    if (it.first == Resources::IRON) 
      continue;
    // Add oxygen based on oxygen-boast and current oxygen.
    else if (it.first == Resources::OXYGEN) {
      it.second.first += oxygen_boast_* Faktor(100, total_oxygen_) * 1;
      total_oxygen_ = bound_oxygen_ + it.second.first;
    }
    // Add other resources based on current oxygen.
    else if (it.second.second)
      it.second.first += log(cur_oxygen+1) * Faktor(70, it.second.first) * 1;

    // Add iron (every 2.5sec and randomly.
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
  else if (resources_[Resources::IRON].first < 2 || resources_[resource].second) {
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
  // Get costs for desired unit
  Costs needed = initial_costs_.at(unit);

  // Check costs and add to missing.
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

void Player::AddNeuron(Position pos, int neuron) {
  std::unique_lock ul(mutex_all_neurons_);
  TakeResources(initial_costs_.at(neuron));
  all_neurons_.insert(pos);

  if (neuron == Units::ACTIVATEDNEURON)
    activated_neurons_[pos] = ActivatedNeuron(pos);
  else if (neuron == Units::SYNAPSE)
    synapses_[pos] = Synapse(pos);
}

void Player::AddPotential(Position pos, std::list<Position> way, int unit) {
  // Take resources.
  TakeResources(initial_costs_.at(unit));

  // Add soldier
  std::unique_lock ul(mutex_potentials_);
  if (unit == Units::EPSP)
    epsps_[utils::create_id()] = Epsp(pos, way);
  else if (unit == Units::IPSP) {
    ipsps_[utils::create_id()] = Ipsp(pos, way, 5);
    std::string msg = "Added ipsp with way size: " + std::to_string(way.size());
    mvaddstr(1, 0, msg.c_str());
  }
}

bool Player::AddTechnology(int technology) {
  std::unique_lock ul(mutex_technologies_);
  if (technologies_.count(technology) > 0) {
    technologies_[technology].first++;
    return true;
  }
  return false;
}

int Player::MovePotential(Position target_neuron, Player* enemy) {
  // Move soldiers along the way to it's target and check if target is reached.
  std::shared_lock sl_potenial(mutex_potentials_);
  std::vector<std::string> potential_to_remove;
  auto cur_time = std::chrono::steady_clock::now();

  // Move epsps and collect damage.
  int damage = 0;
  for (auto& it : epsps_) {
    // Only move potential, if way not empty and recharge is done.
    if (it.second.way_.size() > 0 && utils::get_elapsed(it.second.last_action_, cur_time) > it.second.speed_) {
      it.second.pos_ = it.second.way_.front(); 
      it.second.way_.pop_front();
      if (it.second.pos_ == target_neuron) {
        damage+=it.second.attack_;  // add potential to damage.
        potential_to_remove.push_back(it.first); // remove 
      }
      it.second.last_action_ = cur_time;  // potential did action, so update last_action_.
    }
  }

  // Move ipsps.
  for (auto& it : ipsps_) {
    // Move potential if not already at target position.
    if (it.second.pos_ != target_neuron && it.second.way_.size() > 0
        && utils::get_elapsed(it.second.last_action_, cur_time) > it.second.speed_) {
      it.second.pos_ = it.second.way_.front(); 
      it.second.way_.pop_front();
      it.second.last_action_ = cur_time;  // potential did action, so update last_action_.
    }
    // Otherwise, check if waiting time is up.
    else if (utils::get_elapsed(it.second.last_action_, cur_time) > it.second.duration_*1000) {
      enemy->SetBlockForNeuron(target_neuron, Units::ACTIVATEDNEURON, false);
      potential_to_remove.push_back(it.first); // remove 
    }
    else if (it.second.pos_ == target_neuron) {
      enemy->SetBlockForNeuron(target_neuron, Units::ACTIVATEDNEURON, true);
    }
  }
  sl_potenial.unlock();

  // Remove potential which has reached it's target.
  std::unique_lock ul_potential(mutex_potentials_);
  for (const auto& it : potential_to_remove) {
    if (epsps_.count(it) > 0)
      epsps_.erase(it);
    else if (ipsps_.count(it) > 0)
      ipsps_.erase(it);
  }
  return damage;
}

void Player::SetBlockForNeuron(Position pos, int unit, bool blocked) {
  std::unique_lock ul(mutex_all_neurons_);
  if (unit == Units::ACTIVATEDNEURON && activated_neurons_.count(pos) > 0) {
    activated_neurons_[pos].blocked_ = blocked;
  }
}

void Player::HandleDef(Player* enemy) {
  std::shared_lock sl(mutex_all_neurons_);
  auto cur_time = std::chrono::steady_clock::now();
  for (auto& activated_neuron : activated_neurons_) {
    // Check if tower's recharge is done.
    if (utils::get_elapsed(activated_neuron.second.last_action_, cur_time) > activated_neuron.second.speed_
        && !activated_neuron.second.blocked_) {
      for (const auto& potential : enemy->epsps()) {
        int distance = utils::dist(activated_neuron.first, potential.second.pos_);
        if (distance < 3) {
          // Decrease potential and removes potential if potential is down to zero.
          enemy->NeutalizePotential(potential.first);
          activated_neuron.second.last_action_ = cur_time;  // tower did action, so update last_action_.
          break;  // break loop, since every activated neuron only neutralizes one potential.
        }
      }
      for (const auto& potential : enemy->ipsps()) {
        int distance = utils::dist(activated_neuron.first, potential.second.pos_);
        if (distance < 3) {
          // Decrease potential and removes potential if potential is down to zero.
          enemy->NeutalizePotential(potential.first);
          activated_neuron.second.last_action_ = cur_time;  // tower did action, so update last_action_.
          break;  // break loop, since every activated neuron only neutralizes one potential.
        }
      }

    }
  }
}

void Player::NeutalizePotential(std::string id) {
  std::unique_lock ul(mutex_potentials_);
  if (epsps_.count(id) > 0) {
    epsps_[id].attack_--;
    if (epsps_[id].attack_ == 0)
      epsps_.erase(id);
  }
  if (ipsps_.count(id) > 0) {
    ipsps_[id].attack_--;
    if (ipsps_[id].attack_ == 0)
      ipsps_.erase(id);
  }
}

bool Player::IsSoldier(Position pos, int unit) {
  std::shared_lock sl(mutex_potentials_);
  if (unit == -1 || unit == Units::EPSP) {
    for (const auto& it : epsps_) {
      if (it.second.pos_ == pos)
        return true;
    }
  }
  if (unit == -1 || unit == Units::IPSP) {
    for (const auto& it : ipsps_) {
      if (it.second.pos_ == pos)
        return true;
    }
  }
  return false;
}

bool Player::IsActivatedResource(int resource) {
  return resources_.at(resource).second;
}

bool Player::IncreaseNeuronPotential(int val, int neuron) {
  std::unique_lock ul(mutex_nucleus_);
  if (neuron == Units::NUCLEUS) {
    nucleus_.lp_ += val;
    if (nucleus_.lp_ >= nucleus_.max_lp_)
      return true;
  }
  return false;
}
