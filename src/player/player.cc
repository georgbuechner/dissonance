#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cwchar>
#include <exception>
#include <math.h>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>

#include "constants/codes.h"
#include "constants/costs.h"
#include "objects/transfer.h"
#include "objects/units.h"
#include "spdlog/logger.h"
#include "curses.h"
#include "game/field.h"
#include "player/player.h"
#include "utils/utils.h"

#define LOGGER "logger"

#define HILL ' ' 
#define DEN 'D' 
#define GOLD 'G' 
#define SILVER 'S' 
#define BRONZE 'B' 
#define FREE char(46)
#define DEF 'T'

Player::Player(position_t nucleus_pos, Field* field, RandomGenerator* ran_gen) 
    : cur_range_(4), resource_slowdown_(3) {
  field_ = field;
  ran_gen_ = ran_gen;

  neurons_[nucleus_pos] = std::make_unique<Nucleus>(nucleus_pos);
  auto r_pos= field_->AddResources(nucleus_pos);
  // Max only 20 as iron should be rare.
  resources_.insert(std::pair<int, Resource>(IRON, Resource(3, 22, 2, true, {-1, -1})));  
  resources_.insert(std::pair<int, Resource>(Resources::OXYGEN, Resource(5.5, 100, 0, false, r_pos[OXYGEN]))); 
  resources_.insert(std::pair<int, Resource>(Resources::POTASSIUM, Resource(0, 100, 0, false, r_pos[POTASSIUM]))); 
  resources_.insert(std::pair<int, Resource>(Resources::CHLORIDE, Resource(0, 100, 0, false, r_pos[CHLORIDE]))); 
  // Max 150: allows 7 activated neurons withput updates.
  resources_.insert(std::pair<int, Resource>(Resources::GLUTAMATE, Resource(0, 150, 0, false, r_pos[GLUTAMATE]))); 
  // Max low, dopamine is never bound.
  resources_.insert(std::pair<int, Resource>(Resources::DOPAMINE, Resource(0, 70, 0, false, r_pos[DOPAMINE]))); 
  // Max low, as serotonin is never bound.
  resources_.insert(std::pair<int, Resource>(Resources::SEROTONIN, Resource(0, 70, 0, false, r_pos[SEROTONIN]))); 
  
  technologies_ = {
    {UnitsTech::WAY, {0,3}},
    {UnitsTech::SWARM, {0,3}},
    {UnitsTech::TARGET, {0,2}},
    {UnitsTech::TOTAL_RESOURCE, {0,3}},
    {UnitsTech::CURVE, {0,3}},
    {UnitsTech::ATK_POTENIAL, {0,3}},
    {UnitsTech::ATK_SPEED, {0,3}},
    {UnitsTech::ATK_DURATION, {0,3}},
    {UnitsTech::DEF_POTENTIAL, {0,3}},
    {UnitsTech::DEF_SPEED, {0,3}},
    {UnitsTech::NUCLEUS_RANGE, {0,3}},
  };
}

std::vector<std::string> Player::GetCurrentStatusLine() {
  std::shared_lock sl(mutex_resources_);
  std::string end = ": ";
  return { 
    "RESOURCES",
    "",
    "slowdown: ", std::to_string(resource_slowdown_), "",
    "resources format: ", "[free]+[bound]/[limit]", "++[boost]", "",
    "Iron " SYMBOL_IRON + end, resources_.at(IRON).Print(), "",
    "oxygen: ", resources_.at(OXYGEN).Print(),
      "+" + utils::Dtos(resources_.at(OXYGEN).distributed_iron()), "",
    "potassium " SYMBOL_POTASSIUM + end, resources_.at(POTASSIUM).Print(), 
      "+" + utils::Dtos(resources_.at(POTASSIUM).distributed_iron()), "",
    "chloride " SYMBOL_CHLORIDE + end, resources_.at(CHLORIDE).Print(),
      "+" + utils::Dtos(resources_.at(CHLORIDE).distributed_iron()), "",
    "glutamate " SYMBOL_GLUTAMATE + end, resources_.at(GLUTAMATE).Print(),
      "+" + utils::Dtos(resources_.at(GLUTAMATE).distributed_iron()), "",
    "dopamine " SYMBOL_DOPAMINE + end, resources_.at(DOPAMINE).Print(),
      "+" + utils::Dtos(resources_.at(DOPAMINE).distributed_iron()), "",
    "serotonin " SYMBOL_SEROTONIN + end, resources_.at(SEROTONIN).Print(),
      "+" + utils::Dtos(resources_.at(SEROTONIN).distributed_iron()), "",
    "nucleus " SYMBOL_DEN " potential" + end, GetNucleusLive()
  };
}

// getter 
std::map<std::string, Potential> Player::potential() { 
  std::shared_lock sl(mutex_potentials_);
  return potential_; 
}

position_t Player::GetOneNucleus() { 
  std::shared_lock sl(mutex_all_neurons_); 
  auto all_nucleus_positions = GetAllPositionsOfNeurons(NUCLEUS);
  if (all_nucleus_positions.size() > 0)
    return all_nucleus_positions.front();
  return {-1, -1};
}

int Player::cur_range() { 
  return cur_range_;
}

std::map<int, Resource> Player::resources() {
  std::shared_lock sl(mutex_resources_);
  return resources_;
}

std::map<int, tech_of_t> Player::technologies() {
  std::shared_lock sl(mutex_technologies_);
  return technologies_;
}
std::map<int, Transfer::Resource> Player::t_resources() {
  std::map<int, Transfer::Resource>  resources;
  for (const auto& it : resources_) 
    resources[it.first] = {utils::Dtos(it.second.cur()), utils::Dtos(it.second.bound()), 
      std::to_string(it.second.limit()), std::to_string(it.second.distributed_iron()), it.second.Active()};
  return resources;
}
std::map<int, Transfer::Technology> Player::t_technologies() {
  std::map<int, Transfer::Technology> technologies;
  for (const auto& it : technologies_) 
    technologies[it.first] = {std::to_string(it.second.first), std::to_string(it.second.second), it.second.first > 0};
  return technologies;
}

// setter 
void Player::set_enemies(std::vector<Player*> enemies) {
  enemies_ = enemies;
}

// methods 

position_t Player::GetPositionOfClosestNeuron(position_t pos, int unit) {
  spdlog::get(LOGGER)->info("Player::GetPositionOfClosestNeuron");
  std::shared_lock sl(mutex_all_neurons_);
  int min_dist = 999;
  position_t closest_nucleus_pos = {-1, -1}; 
  for (const auto& it : neurons_) {
    if (it.second->type_ != unit)
      continue;
    int dist = utils::Dist(pos, it.first);
    if(dist < min_dist) {
      closest_nucleus_pos = it.first;
      min_dist = dist;
    }
  }
  spdlog::get(LOGGER)->info("Player::GetPositionOfClosestNeuron: done");
  return closest_nucleus_pos;
}

std::string Player::GetNucleusLive() {
  spdlog::get(LOGGER)->info("Player::GetNucleusLive");
  std::shared_lock sl(mutex_all_neurons_);
  auto positions = GetAllPositionsOfNeurons(NUCLEUS);
  if (positions.size() > 0) {
    return std::to_string(neurons_.at(positions.front())->voltage()) 
      + " / " + std::to_string(neurons_.at(positions.front())->max_voltage());
  }
  return "---";
}

bool Player::HasLost() {
  std::shared_lock sl(mutex_all_neurons_);
  return GetAllPositionsOfNeurons(NUCLEUS).size() == 0;
}

int Player::GetNeuronTypeAtPosition(position_t pos) {
  std::shared_lock sl(mutex_all_neurons_);
  if (neurons_.count(pos) > 0)
    return neurons_.at(pos)->type_;
  return -1;
}

bool Player::IsNeuronBlocked(position_t pos) {
  std::shared_lock sl(mutex_all_neurons_);
  if (neurons_.count(pos) > 0)
    return neurons_.at(pos)->blocked();
  return false;
}

std::vector<position_t> Player::GetAllPositionsOfNeurons(int type) {
  std::shared_lock sl(mutex_all_neurons_);
  std::vector<position_t> positions;
  for (const auto& it : neurons_)
    if (type == -1 || it.second->type_ == type)
      positions.push_back(it.first);
  return positions;
}

position_t Player::GetRandomNeuron(std::vector<int>) {
  spdlog::get(LOGGER)->info("Player::GetRandomNeuron");
  std::shared_lock sl(mutex_all_neurons_);
  // Get all positions at which there are activated neurons
  std::vector<position_t> activated_neuron_postions;
  for (const auto& it : neurons_)
    activated_neuron_postions.push_back(it.first);
  // If none return invalied position.
  if (activated_neuron_postions.size() == 0)
    return {-1, -1};
  // If only one, return this position.
  if (activated_neuron_postions.size() == 1)
    return activated_neuron_postions.front();
  // Otherwise, get random index and return position at index.
  int ran = ran_gen_->RandomInt(0, activated_neuron_postions.size()-1);
  spdlog::get(LOGGER)->info("Player::GetRandomNeuron: done");
  return activated_neuron_postions[ran];
}

int Player::ResetWayForSynapse(position_t pos, position_t way_position) {
  spdlog::get(LOGGER)->info("Player::ResetWayForSynapse");
  std::unique_lock ul(mutex_all_neurons_);
  if (neurons_.count(pos) && neurons_.at(pos)->type_ == UnitsTech::SYNAPSE) {
    neurons_.at(pos)->set_way_points({way_position});
    spdlog::get(LOGGER)->info("Player::ResetWayForSynapse: successfully");
    return neurons_.at(pos)->ways_points().size();
  }
  else {
    spdlog::get(LOGGER)->warn("Player::ResetWayForSynapse: neuron not found or wrong type");
    return -1;
  }
}

int Player::AddWayPosForSynapse(position_t pos, position_t way_position) {
  spdlog::get(LOGGER)->info("Player::AddWayPosForSynapse");
  std::unique_lock ul(mutex_all_neurons_);
  if (neurons_.count(pos) && neurons_.at(pos)->type_ == UnitsTech::SYNAPSE) {
    auto cur_way = neurons_.at(pos)->ways_points();
    cur_way.push_back(way_position);
    neurons_.at(pos)->set_way_points(cur_way);
    spdlog::get(LOGGER)->info("Player::AddWayPosForSynapse: successfully");
    return cur_way.size();
  }
  else {
    spdlog::get(LOGGER)->warn("Player::AddWayPosForSynapse: neuron not found or wrong type");
    return -1;
  }
}

void Player::SwitchSwarmAttack(position_t pos) {
  spdlog::get(LOGGER)->info("Player::SwitchSwarmAttack");
  std::unique_lock ul(mutex_all_neurons_);
  if (neurons_.count(pos) && neurons_.at(pos)->type_ == UnitsTech::SYNAPSE)
    neurons_.at(pos)->set_swarm(!neurons_.at(pos)->swarm());
  spdlog::get(LOGGER)->info("Player::SwitchSwarmAttack: done");
}

void Player::ChangeIpspTargetForSynapse(position_t pos, position_t target_pos) {
  spdlog::get(LOGGER)->info("Player::ChangeIpspTargetForSynapse");
  std::unique_lock ul(mutex_all_neurons_);
  if (neurons_.count(pos) && neurons_.at(pos)->type_ == UnitsTech::SYNAPSE)
    neurons_.at(pos)->set_ipsp_target_pos(target_pos);
  spdlog::get(LOGGER)->info("Player::ChangeIpspTargetForSynapse: done");
}

void Player::ChangeEpspTargetForSynapse(position_t pos, position_t target_pos) {
  spdlog::get(LOGGER)->info("Player::ChangeEpspTargetForSynapse");
  std::unique_lock ul(mutex_all_neurons_);
  if (neurons_.count(pos) && neurons_.at(pos)->type_ == UnitsTech::SYNAPSE)
    neurons_.at(pos)->set_epsp_target_pos(target_pos);
  spdlog::get(LOGGER)->info("Player::ChangeEpspTargetForSynapse: done");
}

void Player::IncreaseResources(bool inc_iron) {
  spdlog::get(LOGGER)->info("Player::IncreaseResources");
  std::unique_lock ul_resources(mutex_resources_);
  double gain = std::abs(log(resources_.at(Resources::OXYGEN).cur()+0.5));
  for (auto& it : resources_) {
    // Inc only if min 2 iron is distributed, inc iron only depending on audio.
    if (it.second.Active() && !it.second.blocked() && (it.first != IRON || inc_iron))  
      it.second.IncreaseResource(gain, resource_slowdown_);
  }
  spdlog::get(LOGGER)->info("Player::IncreaseResources: done");
}

bool Player::DistributeIron(int resource) {
  spdlog::get(LOGGER)->info("Player::DistributeIron: resource={}", resource);
  std::unique_lock ul(mutex_resources_);
  if (resources_.count(resource) == 0 || resource == IRON) {
    spdlog::get(LOGGER)->error("Player::DistributeIron: invalid resource!");
    return false;
  }
  else if (resources_.at(IRON).cur() < 1) {
    spdlog::get(LOGGER)->info("Player::DistributeIron: not enough iron!");
    return false;
  }
  int active_before = resources_.at(resource).Active();
  resources_.at(resource).set_distribited_iron(resources_.at(resource).distributed_iron()+1);
  int active_after = resources_.at(resource).Active();
  if (active_after && !active_before)
    AddNeuron(resources_.at(resource).pos(), RESOURCENEURON);  // Add resource neuron.


  resources_.at(IRON).set_cur(resources_.at(IRON).cur() - 1);
  resources_.at(IRON).set_bound(resources_.at(IRON).bound() + 1);

  spdlog::get(LOGGER)->info("Player::DistributeIron: success!");
  return true;
}

bool Player::RemoveIron(int resource) {
  spdlog::get(LOGGER)->info("Player::RemoveIron: resource={}", resource);
  std::unique_lock ul(mutex_resources_);
  if (resources_.count(resource) == 0 || resource == IRON) {
    spdlog::get(LOGGER)->error("Player::RemoveIron: invalid resource!");
    return false;
  }
  if (resources_.at(resource).distributed_iron() == 0) {
    spdlog::get(LOGGER)->error("Player::RemoveIron: no iron distributed to this resource!");
    return false;
  }
  int active_before = resources_.at(resource).Active();
  resources_.at(resource).set_distribited_iron(resources_.at(resource).distributed_iron()-1);
  int active_after = resources_.at(resource).Active();
  if (!active_after && active_before)
    AddPotentialToNeuron(resources_.at(resource).pos(), 100);  // Remove resource neuron.
  resources_.at(IRON).set_cur(resources_.at(IRON).cur() + 1);
  resources_.at(IRON).set_bound(resources_.at(IRON).bound() -1);
  spdlog::get(LOGGER)->info("Player::RemoveIron: success!");
  return true;
}

Costs Player::GetMissingResources(int unit, int boast) {
  std::shared_lock sl(mutex_resources_);
  // Get costs for desired unit
  Costs needed = units_costs_.at(unit);

  // Check costs and add to missing.
  std::map<int, double> missing;
  for (const auto& it : needed)
    if (resources_.at(it.first).cur() < it.second*boast) 
      missing[it.first] = it.second - resources_.at(it.first).cur();
  return missing;
}

bool Player::TakeResources(int type, bool bind_resources, int boast) {
  spdlog::get(LOGGER)->info("Player::TakeResources");
  if (units_costs_.count(type) == 0) {
    spdlog::get(LOGGER)->info("Player::TakeResources: done as free resource.");
    return true;
  }
  if (GetMissingResources(type, boast).size() > 0) {
    spdlog::get(LOGGER)->warn("Player::TakeResources: taking resources with out enough resources!");
    return false;
  }
  std::unique_lock ul_resources(mutex_resources_);
  auto costs = units_costs_.at(type);
  for (const auto& it : costs) {
    resources_.at(it.first).set_cur(resources_.at(it.first).cur() - it.second*boast);
    if (bind_resources)
      resources_.at(it.first).set_bound(resources_.at(it.first).bound() + it.second*boast);
  }
  spdlog::get(LOGGER)->info("Player::TakeResources: done");
  return true;
}

bool Player::AddNeuron(position_t pos, int neuron_type, position_t epsp_target, position_t ipsp_target) {
  spdlog::get(LOGGER)->info("Player::AddNeuron");
  std::unique_lock ul(mutex_all_neurons_);
  if (!TakeResources(neuron_type, true))
    return false;
  if (neuron_type == UnitsTech::ACTIVATEDNEURON) {
    spdlog::get(LOGGER)->debug("Player::AddNeuron: ActivatedNeuron");
    std::shared_lock sl(mutex_technologies_);
    int speed_boast = technologies_.at(UnitsTech::DEF_SPEED).first * 40;
    int potential_boast = technologies_.at(UnitsTech::DEF_POTENTIAL).first;
    neurons_[pos] = std::make_unique<ActivatedNeuron>(pos, potential_boast, speed_boast);
  }
  else if (neuron_type == UnitsTech::SYNAPSE) {
    spdlog::get(LOGGER)->debug("Player::AddNeuron: Synapse");
    neurons_[pos] = std::make_unique<Synapse>(pos, technologies_.at(UnitsTech::SWARM).first*3+1, 
        technologies_.at(UnitsTech::WAY).first, epsp_target, ipsp_target);
    spdlog::get(LOGGER)->debug("Player::AddNeuron, created synapse, {}", neurons_.at(pos)->type_);
  }
  else if (neuron_type == UnitsTech::NUCLEUS) {
    spdlog::get(LOGGER)->debug("Player::AddNeuron: Nucleus");
    neurons_[pos] = std::make_unique<Nucleus>(pos);
    UpdateResourceLimits(0.1); // Increase max resource if new nucleus is built.
  }
  else if (neuron_type == UnitsTech::RESOURCENEURON) {
    spdlog::get(LOGGER)->debug("Player::AddNeuron: ResourceNeuron");
    std::string symbol = field_->GetSymbolAtPos(pos);
    spdlog::get(LOGGER)->debug("Player::AddNeuron: got symbol: {}", symbol);
    int resource_type = resources_symbol_mapping.at(symbol);
    spdlog::get(LOGGER)->debug("Player::AddNeuron: got resource_type: {}", resource_type);
    neurons_[pos] = std::make_unique<ResourceNeuron>(pos, resource_type);
    spdlog::get(LOGGER)->debug("Player::AddNeuron, Created resourceneuron, {}", neurons_.at(pos)->type_);
  }
  spdlog::get(LOGGER)->info("Player::AddNeuron: done");
  return true;
}

bool Player::AddPotential(position_t synapes_pos, int unit) {
  spdlog::get(LOGGER)->info("Player::AddPotential.");
  if (!TakeResources(unit, false))
    return false;
  // Get way and target:
  std::shared_lock sl_neurons(mutex_all_neurons_);
  // Check if synapses is blocked.
  if (neurons_.count(synapes_pos) == 0 || neurons_.at(synapes_pos)->type_ != SYNAPSE 
      || neurons_.at(synapes_pos)->blocked()) 
    return true;
  
  // Create way.
  spdlog::get(LOGGER)->debug("Player::AddPotential: get way for potential.");
  neurons_.at(synapes_pos)->UpdateIpspTargetIfNotSet(enemies_.front()->GetRandomNeuron());
  auto way = field_->GetWayForSoldier(synapes_pos, neurons_.at(synapes_pos)->GetWayPoints(unit));

  // Add potential.
  std::unique_lock ul_potentials(mutex_potentials_);
  std::shared_lock sl_technologies(mutex_technologies_);
  // Get boast from technologies.
  int potential_boast = technologies_.at(UnitsTech::ATK_POTENIAL).first;
  int speed_boast = 50*technologies_.at(UnitsTech::ATK_POTENIAL).first;
  int duration_boast = technologies_.at(UnitsTech::ATK_DURATION).first;
  sl_technologies.unlock();
  if (unit == UnitsTech::EPSP) {
    spdlog::get(LOGGER)->debug("Player::AddPotential: epsp - get num epsps to create.");
    // Increase num of currently stored epsps and get number of epsps to create.
    size_t num_epsps_to_create = neurons_.at(synapes_pos)->AddEpsp();
    spdlog::get(LOGGER)->debug("Player::AddPotential: epsp - creating {} epsps.", num_epsps_to_create);
    for (size_t i=0; i<num_epsps_to_create; i++)
      potential_[utils::CreateId("epsp")] = Epsp(synapes_pos, way, potential_boast, speed_boast);
  }
  else if (unit == UnitsTech::IPSP) {
    spdlog::get(LOGGER)->debug("Player::AddPotential: ipsp - creating 1 ipsp.");
    potential_[utils::CreateId("ipsp")] = Ipsp(synapes_pos, way, potential_boast, speed_boast, duration_boast);
  }
  spdlog::get(LOGGER)->info("Player::AddPotential: done.");
  return true;
}

bool Player::AddTechnology(int technology) {
  std::unique_lock ul(mutex_technologies_);
  spdlog::get(LOGGER)->info("Player::AddTechnology.");

  // Check if technology exists, resources are missing and whether already fully researched.
  if (technologies_.count(technology) == 0)
    return false;
  if (GetMissingResources(technology, technologies_[technology].first+1).size() > 0 
      || technologies_[technology].first == technologies_[technology].second)
    return false;
  if (!TakeResources(technology, false, technologies_[technology].first+1))
    return false;
 
  // Handle technology.
  spdlog::get(LOGGER)->debug("Player::AddTechnology: Adding new technology");
  std::unique_lock ul_resources(mutex_resources_);
  technologies_[technology].first++;
  if (technology == UnitsTech::WAY) {
    for (auto& it : neurons_)
      if (it.second->type_ == UnitsTech::SYNAPSE)
        it.second->set_availible_ways(technologies_[technology].first);
  }
  else if (technology == UnitsTech::SWARM) {
    for (auto& it : neurons_)
      if (it.second->type_ == UnitsTech::SYNAPSE)
        it.second->set_max_stored(technologies_[technology].first*3+1);
  }
  else if (technology == UnitsTech::TOTAL_RESOURCE) {
    ul_resources.unlock();
    UpdateResourceLimits(0.2);
    ul_resources.lock();
  }
  else if (technology == UnitsTech::CURVE) {
    if (technologies_[technology].first == 3)
      resource_slowdown_ = 0.5;
    else 
      resource_slowdown_--;
  }
  else if (technology == UnitsTech::NUCLEUS_RANGE)
    cur_range_++;
  spdlog::get(LOGGER)->info("Player::AddTechnology: success");
  return true;
}

void Player::MovePotential() {
  spdlog::get(LOGGER)->info("Player::MovePotential");
  // Move soldiers along the way to it's target and check if target is reached.
  std::shared_lock sl_potenial(mutex_potentials_);
  std::vector<std::string> potential_to_remove;
  auto cur_time = std::chrono::steady_clock::now();
  for (auto& it : potential_) {
    // If target not yet reached and it is time for the next action, move potential
    if (it.second.way_.size() > 0 && utils::GetElapsed(it.second.last_action_, cur_time) > it.second.speed_) {
      // Move potential.
      it.second.pos_ = it.second.way_.front(); 
      it.second.way_.pop_front();
      it.second.last_action_ = cur_time;  // potential did action, so update last_action_.
    }
    // If target is reached, handle epsp and ipsp seperatly.
    // Epsp: add potential to target and add epsp to list of potentials to remove.
    if (it.second.type_ == UnitsTech::EPSP) {
      if (it.second.way_.size() == 0) {
        for (const auto& enemy : enemies_) {
          if (enemy->GetNeuronTypeAtPosition(it.second.pos_) != -1) {
            enemy->AddPotentialToNeuron(it.second.pos_, it.second.potential_);
            field_->AddBlink(it.second.pos_);
            potential_to_remove.push_back(it.first); // remove 
          }
        }
      }
    }
    // Ipsp: check if just time is up -> remove ipsp, otherwise -> block target.
    else {
      // If duration since last action is reached, add ipsp to list of potentials to remove and unblock target.
      if (utils::GetElapsed(it.second.last_action_, cur_time) > it.second.duration_*1000) {
        for (const auto& enemy : enemies_) {
          if (enemy->GetNeuronTypeAtPosition(it.second.pos_) != -1)
            enemy->SetBlockForNeuron(it.second.pos_, false);  // unblock target.
        }
        potential_to_remove.push_back(it.first); // remove 
      }
      else if (it.second.way_.size() == 0) {
        for (const auto& enemy : enemies_) {
          if (enemy->GetNeuronTypeAtPosition(it.second.pos_) != -1)
            enemy->SetBlockForNeuron(it.second.pos_, true);  // block target
        }
      }
    }
  }
  sl_potenial.unlock();

  // Remove potential which has reached it's target.
  std::unique_lock ul_potential(mutex_potentials_);
  for (const auto& it : potential_to_remove)
    potential_.erase(it);
}

void Player::SetBlockForNeuron(position_t pos, bool blocked) {
  spdlog::get(LOGGER)->info("Player::SetBlockForNeuron");
  std::unique_lock ul(mutex_all_neurons_);
  if (neurons_.count(pos) > 0) {
    neurons_[pos]->set_blocked(blocked);
    // If resource neuron, block/ unblock resource.
    if (neurons_[pos]->type_ == RESOURCENEURON)
      resources_.at(neurons_[pos]->resource()).set_blocked(blocked);
  }
  spdlog::get(LOGGER)->info("Player::SetBlockForNeuron: done");
}

void Player::HandleDef() {
  spdlog::get(LOGGER)->info("Player::HandleDef");
  std::unique_lock ul(mutex_all_neurons_);
  auto cur_time = std::chrono::steady_clock::now();
  for (auto& neuron : neurons_) {
    if (neuron.second->type_ != UnitsTech::ACTIVATEDNEURON)
      continue;
    // Check if activated neurons recharge is done.
    if (utils::GetElapsed(neuron.second->last_action(), cur_time) > neuron.second->speed() 
        && !neuron.second->blocked()) {
      // for every enemy
      for (const auto& enemy : enemies_) {
        // Check for potentials in range of activated neuron.
        for (const auto& potential : enemy->potential()) {
          int distance = utils::Dist(neuron.first, potential.second.pos_);
          if (distance < 3) {
            enemy->NeutralizePotential(potential.first, neuron.second->potential_slowdown());
            field_->AddBlink(potential.second.pos_);
            neuron.second->set_last_action(cur_time);  // neuron did action, so update last_action_.
            break;
          }
        }
      }
    }
  }
}

void Player::NeutralizePotential(std::string id, int potential) {
  spdlog::get(LOGGER)->info("Player::NeutralizePotential: {}", potential);
  std::unique_lock ul(mutex_potentials_);
  if (potential_.count(id) > 0) {
    spdlog::get(LOGGER)->debug("Player::NeutralizePotential: left potential: {}", 
        potential_.at(id).potential_);
    potential_.at(id).potential_ -= potential;
    // Remove potential only if not already at it's target (length of way is greater than zero).
    if (potential_.at(id).potential_ == 0 && potential_.at(id).way_.size() > 0) {
      spdlog::get(LOGGER)->debug("Player::NeutralizePotential: deleting potential...");
      potential_.erase(id);
      spdlog::get(LOGGER)->debug("Player::NeutralizePotential: done.");
    }
  }
  spdlog::get(LOGGER)->info("Player::NeutralizePotential: done.");
}

void Player::AddPotentialToNeuron(position_t pos, int potential) {
  std::unique_lock ul_all_neurons(mutex_all_neurons_);
  spdlog::get(LOGGER)->info("Player::AddPotentialToNeuron: {} {}", utils::PositionToString(pos), potential);
  if (potential < 0) {
    spdlog::get(LOGGER)->warn("Player::AddPotentialToNeuron: negative potential!");
    return;
  }

  if (neurons_.count(pos) > 0) {
    spdlog::get(LOGGER)->debug("Player::AddPotentialToNeuron: left potential: {}", neurons_[pos]->voltage());
    if (neurons_.at(pos)->IncreaseVoltage(potential)) {
      int type = neurons_.at(pos)->type_;
      spdlog::get(LOGGER)->debug("Player::AddPotentialToNeuron: erasing {}", type);
      neurons_.erase(pos);
      spdlog::get(LOGGER)->debug("Player::AddPotentialToNeuron: erasing done");
      // Potentially deactivate all neurons formally in range of the destroyed nucleus.
      if (type == UnitsTech::NUCLEUS) {
        ul_all_neurons.unlock();
        CheckNeuronsAfterNucleusDies();
        ul_all_neurons.lock();
        UpdateResourceLimits(-0.1);  // Remove added max resources when nucleus dies.
      }
      spdlog::get(LOGGER)->debug("Player::AddPotentialToNeuron: adding potential finished.");
    }
  }
  spdlog::get(LOGGER)->info("Player::AddPotentialToNeuron: done");
}

void Player::CheckNeuronsAfterNucleusDies() {
  spdlog::get(LOGGER)->info("Player::CheckNeuronsAfterNucleusDies");
  // Get all nucleus.
  std::vector<position_t> all_nucleus = GetAllPositionsOfNeurons(UnitsTech::NUCLEUS);
  std::shared_lock sl(mutex_all_neurons_);
  std::vector<position_t> neurons_to_remove;
  for (const auto& neuron : neurons_) {
    // Nucleus are not affekted.
    if (neuron.second->type_ == UnitsTech::NUCLEUS)
      continue;
    // Add first and if in range of a nucleus, remove again.
    neurons_to_remove.push_back(neuron.first);
    for (const auto& nucleus_pos : all_nucleus)
      if (utils::Dist(neuron.first, nucleus_pos) <= cur_range_)
        neurons_to_remove.pop_back();
  }
  sl.unlock();
  std::unique_lock ul(mutex_all_neurons_);
  for (const auto& it : neurons_to_remove) {
    neurons_.erase(it);
  }
  spdlog::get(LOGGER)->info("Player::CheckNeuronsAfterNucleusDies: done");
}

std::string Player::GetPotentialIdIfPotential(position_t pos, int unit) {
  std::shared_lock sl(mutex_potentials_);
  for (const auto& it : potential_)
    if (it.second.pos_ == pos && (unit == -1 || it.second.type_ == unit))
      return it.first;
  return "";
}

choice_mapping_t Player::GetOptionsForSynapes(position_t pos) {
  spdlog::get(LOGGER)->info("Player::GetOptionsForSynapes");
  std::shared_lock sl_technologies(mutex_technologies_);
  choice_mapping_t mapping;

  if (neurons_.count(pos) == 0 || neurons_.at(pos)->type_ != SYNAPSE) {
    spdlog::get(LOGGER)->warn("Player::GetOptionsForSynapes: neuron at position does'n exist, or is no synapse: {}.", 
        utils::PositionToString(pos));
    return mapping;
  }
  
  mapping[0] = {"(Re-)set way.", (technologies_.at(UnitsTech::WAY).first > 0) ? COLOR_AVAILIBLE : COLOR_DEFAULT};
  mapping[1] = {"Add way-point.", (neurons_.at(pos)->ways_points().size() < neurons_.at(pos)->num_availible_ways()) 
    ? COLOR_AVAILIBLE : COLOR_DEFAULT};
  mapping[2] = {"Select target for ipsp.", (technologies_.at(UnitsTech::TARGET).first > 0) 
    ? COLOR_AVAILIBLE : COLOR_DEFAULT};
  mapping[3] = {"Select target for epsp.", (technologies_.at(UnitsTech::TARGET).first > 1) 
    ? COLOR_AVAILIBLE : COLOR_DEFAULT};
  mapping[4] = {(neurons_.at(pos)->swarm()) ? "Turn swarm-attack off" : "Turn swarm-attack on", 
    (technologies_.at(UnitsTech::SWARM).first > 0) ? COLOR_AVAILIBLE : COLOR_DEFAULT};
  spdlog::get(LOGGER)->info("Player::GetOptionsForSynapes: done");
  return mapping;
}

void Player::UpdateResourceLimits(float faktor) {
  spdlog::get(LOGGER)->info("Player::UpdateResourceLimits");
  std::unique_lock ul(mutex_resources_); 
  for (auto& it : resources_)
    it.second.set_limit(it.second.limit() + it.second.limit()*faktor);
  spdlog::get(LOGGER)->info("Player::Done");
}

std::string Player::GetCurrentResources() {
  std::string msg = "resources: ";
  for (const auto& it : resources()) 
    msg += resources_name_mapping.at(it.first) + ": " + std::to_string(it.second.cur()) + ", ";
  return msg;
}
