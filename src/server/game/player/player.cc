#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cwchar>
#include <exception>
#include <math.h>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>
#include "share/audio/audio.h"
#include "share/constants/codes.h"
#include "share/constants/costs.h"
#include "share/defines.h"
#include "share/objects/units.h"
#include "share/shemes/data.h"
#include "spdlog/logger.h"

#include "server/game/field/field.h"
#include "server/game//player/player.h"
#include "share/tools/utils/utils.h"

Player::Player(std::string username, std::shared_ptr<Field> field, std::shared_ptr<RandomGenerator> ran_gen, int color) 
    : username_(username), cur_range_(4), color_(color), resource_slowdown_(3) {
  field_ = field;
  ran_gen_ = ran_gen;
  lost_ = false;
  macro_ = ran_gen->RandomInt(0, 1);

  // Set player-color and -name in statistics.
  statistics_ = std::make_shared<Statictics>();
  statistics_->set_color(color);
  statistics_->set_player_name(username_);
  
  technologies_ = {
    {UnitsTech::WAY, {0,3}},
    {UnitsTech::SWARM, {0,3}},
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

void Player::SetupNucleusAndResources(position_t nucleus_pos) {
  nucleus_pos_ = nucleus_pos;
  neurons_[nucleus_pos] = std::make_shared<Nucleus>(nucleus_pos);
  CreateNeuron(NUCLEUS, nucleus_pos);
  std::map<int, position_t> r_pos = field_->resource_neurons().at(nucleus_pos);
  resources_.insert(std::pair<int, Resource>(IRON, Resource(3, 22, 2, true, {-1, -1})));  
  resources_.insert(std::pair<int, Resource>(Resources::OXYGEN, Resource(15.5, 100, 0, false, r_pos[OXYGEN]))); 
  resources_.insert(std::pair<int, Resource>(Resources::POTASSIUM, Resource(0, 100, 0, false, r_pos[POTASSIUM]))); 
  resources_.insert(std::pair<int, Resource>(Resources::CHLORIDE, Resource(0, 100, 0, false, r_pos[CHLORIDE]))); 
  resources_.insert(std::pair<int, Resource>(Resources::GLUTAMATE, Resource(0, 150, 0, false, r_pos[GLUTAMATE]))); 
  resources_.insert(std::pair<int, Resource>(Resources::DOPAMINE, Resource(0, 70, 0, false, r_pos[DOPAMINE]))); 
  resources_.insert(std::pair<int, Resource>(Resources::SEROTONIN, Resource(0, 70, 0, false, r_pos[SEROTONIN]))); 
}

// getter 
std::shared_ptr<Statictics> Player::statistics() const {
  return statistics_;
}

std::map<std::string, Potential> Player::potential() const { 
  return potential_; 
}

int Player::cur_range() const { 
  return cur_range_;
}

std::map<int, Resource> Player::resources() const {
  return resources_;
}

std::map<int, tech_of_t> Player::technologies() const {
  return technologies_;
}

std::vector<std::shared_ptr<Player>> Player::enemies() const {
  return enemies_;
}

int Player::color() const {
  return color_;
}

int Player::macro() const { return macro_; }

// setter 
void Player::set_enemies(std::vector<std::shared_ptr<Player>> enemies) {
  enemies_ = enemies;
}

void Player::set_lost(bool lost) {
  lost_ = lost;
}

// methods 

position_t Player::GetSynapesTarget(position_t synape_pos, int unit) const {
  if (neurons_.count(synape_pos) > 0)
    return neurons_.at(synape_pos)->GetTargetForPotential(unit);
  return DEFAULT_POS;
}

std::vector<position_t> Player::GetSynapesWaypoints(position_t synapse_pos, int unit) const {
  if (neurons_.count(synapse_pos) > 0) {
    if (unit == -1)
      return neurons_.at(synapse_pos)->waypoints();
    else 
      return neurons_.at(synapse_pos)->GetWayPoints(unit);
  }
  return {};
}

std::map<int, Data::Resource> Player::GetResourcesInDataFormat() const {
  std::map<int, Data::Resource>  resources;
  for (const auto& it : resources_) 
    resources[it.first] = {it.second.cur(), it.second.bound(), it.second.limit(), it.second.distributed_iron(), 
      it.second.Active()};
  return resources;
}

std::shared_ptr<Statictics> Player::GetFinalStatistics() {
  for (const auto& it : resources_) {
    std::map<std::string, double> res_stats;
    if (it.first != IRON) {
      res_stats["gathered"] = it.second.total();
      res_stats["spent"] = it.second.spent();
      res_stats["ø boost"] = it.second.average_boost();
      res_stats["ø bound"] = it.second.average_bound();
      res_stats["ø neg.-faktor"] = 1-it.second.average_neg_factor();
      res_stats["% active"] = it.second.active_percent();
      statistics_->stats_resources_ref()[it.first] = res_stats;
    }
  }
  statistics_->set_technologies(technologies_);
  return statistics_;
}

std::map<position_t, int> Player::GetNewNeurons() {
  std::map<position_t, int> new_neurons = new_neurons_;
  new_neurons_.clear();
  return new_neurons;
}

std::map<position_t, int> Player::GetNewDeadNeurons() {
  std::map<position_t, int> new_dead_neurons = new_dead_neurons_;
  new_dead_neurons_.clear();
  return new_dead_neurons;
}

int Player::GetNeuronTypeAtPosition(position_t pos) const {
  if (neurons_.count(pos) > 0)
    return neurons_.at(pos)->type_;
  return -1;
}

std::map<position_t, int> Player::GetEpspAtPosition() const {
  std::map<position_t, int> epsps;
  for (const auto& it : potential_) {
    if (it.second.type_ == EPSP) 
      epsps[it.second.pos_]++;
  }
  return epsps;
}

std::map<position_t, int> Player::GetIpspAtPosition() const {
  std::map<position_t, int> ipsps;
  for (const auto& it : potential_) {
    if (it.second.type_ == IPSP) 
      ipsps[it.second.pos_]++;
  }
  return ipsps;
}

std::map<position_t, int> Player::GetMacroAtPosition() const {
  std::map<position_t, int> macros;
  for (const auto& it : potential_) {
    if (it.second.type_ == MACRO) 
      macros[it.second.pos_]++;
  }
  return macros;
}

std::vector<position_t> Player::GetPotentialPositions() const {
  // Gather positions in set, to avoid duplicates.
  std::set<position_t> potential_positions;
  for (const auto& it : potential_)
    potential_positions.insert(it.second.pos_);
  return {potential_positions.begin(), potential_positions.end()};
}

void Player::GetPositionsOfHitPotentials(std::set<position_t>& hits) {
  for (auto& it : potential_) {
    if (it.second._hit) {
      hits.insert(it.second.pos_);
      it.second._hit = false;
    }
  }
}

std::vector<FieldPosition> Player::GetAllNeuronsInRange(position_t pos) const {
  std::vector<FieldPosition> neurons_in_range_;
  for (const auto& it : neurons_) {
    if (utils::Dist(it.first, pos) < cur_range_)
      neurons_in_range_.push_back(FieldPosition(it.first, it.second->type_, color_));
  }
  return neurons_in_range_;
}

std::vector<position_t> Player::GetAllPositionsOfNeurons(int type) const {
  if (neuron_positions_.count(type) > 0)
    return neuron_positions_.at(type);
  return {};
}

position_t Player::GetOneNucleus() const { 
  auto all_nucleus_positions = GetAllPositionsOfNeurons(NUCLEUS);
  if (all_nucleus_positions.size() > 0)
    return all_nucleus_positions.front();
  spdlog::get(LOGGER)->warn("NO NUCLEUS!");
  return DEFAULT_POS;
}

int Player::GetColorForPos(position_t pos) const {
  if (GetNeuronTypeAtPosition(pos) == RESOURCENEURON && resources_.at(neurons_.at(pos)->resource()).Active())
    return COLOR_RESOURCES;
  else if (GetNeuronTypeAtPosition(pos) != -1)
    return color_;
  return COLOR_DEFAULT;
}

position_t Player::GetPositionOfClosestNeuron(position_t pos, int unit) const {
  int min_dist = 999;
  position_t closest_nucleus_pos = {-1, -1}; 
  for (const auto& it : neurons_) {
    if (it.second->type_ != unit)
      continue;
    int dist = utils::Dist(pos, it.first);
    if (dist < min_dist) {
      closest_nucleus_pos = it.first;
      min_dist = dist;
    }
  }
  return closest_nucleus_pos;
}

std::string Player::GetNucleusLive() const {
  auto positions = GetAllPositionsOfNeurons(NUCLEUS);
  // If lost (probably because player quit game) or no nucleus left (player lost game)
  if (lost_ || positions.size() == 0)
    return "---";
  // Otherwise show nucleus-life of first nucleus in list.
  return std::to_string(neurons_.at(positions.front())->voltage()) 
    + " / " + std::to_string(neurons_.at(positions.front())->max_voltage());
}

Costs Player::GetMissingResources(int unit, int boost) const {
  // Check costs and add to missing.
  std::map<int, double> missing;
  for (const auto& it : units_costs_.at(unit))
    if (resources_.at(it.first).cur() < it.second*boost) 
      missing[it.first] = it.second - resources_.at(it.first).cur();
  return missing;
}

bool Player::TakeResources(int type, bool bind_resources, int boost) {
  spdlog::get(LOGGER)->debug("Player::TakeResources");
  if (units_costs_.count(type) == 0) {
    spdlog::get(LOGGER)->debug("Player::TakeResources: done as free resource-neuron or loophole");
    return true;
  }
  if (GetMissingResources(type, boost).size() > 0) {
    spdlog::get(LOGGER)->warn("Player::TakeResources: taking resources without enough resources!");
    return false;
  }
  // Take reesources.
  for (const auto& it : units_costs_.at(type))
    resources_.at(it.first).Decrease(it.second*boost, bind_resources);
  return true;
}

void Player::FreeBoundResources(int type) {
  spdlog::get(LOGGER)->debug("Player::FreeBoundResources: {}", type);
  if (units_costs_.count(type) == 0) {
    if (type != RESOURCENEURON)
      spdlog::get(LOGGER)->warn("Player::FreeBoundResources: attempting to free, but no costs known. Type: {}", type);
    return;
  }
  // Get costs for this unit.
  auto costs = units_costs_.at(type);
  // Free bound resources.
  for (const auto& it : costs)
    resources_.at(it.first).set_bound(resources_.at(it.first).bound() - it.second);
}

bool Player::AddNeuron(position_t pos, int neuron_type, position_t epsp_target) {
  spdlog::get(LOGGER)->debug("Player::AddNeuron: {} {}", neuron_type, utils::PositionToString(pos));
  if (!TakeResources(neuron_type, true))
    return false;
  if (neuron_type == UnitsTech::ACTIVATEDNEURON) {
    spdlog::get(LOGGER)->debug("Player::AddNeuron: ActivatedNeuron");
    int speed_boast = technologies_.at(UnitsTech::DEF_SPEED).first;
    int potential_boast = technologies_.at(UnitsTech::DEF_POTENTIAL).first;
    neurons_[pos] = std::make_shared<ActivatedNeuron>(pos, potential_boast, speed_boast);
  }
  else if (neuron_type == UnitsTech::SYNAPSE) {
    spdlog::get(LOGGER)->debug("Player::AddNeuron: Synapse");
    if (!field_->graph()->InGraph(epsp_target)) {
      spdlog::get(LOGGER)->warn("Player::AddNeuron: Synapse: epsp-target no in graph {}", 
          utils::PositionToString(epsp_target));
      return false;
    }
    neurons_[pos] = std::make_shared<Synapse>(pos, technologies_.at(UnitsTech::SWARM).first*3+1, 
        technologies_.at(UnitsTech::WAY).first, epsp_target, enemies_.front()->GetRandomNeuron());
  }
  else if (neuron_type == UnitsTech::NUCLEUS) {
    spdlog::get(LOGGER)->debug("Player::AddNeuron: Nucleus");
    neurons_[pos] = std::make_shared<Nucleus>(pos);
    UpdateResourceLimits(0.1); // Increase max resource if new nucleus is built.
  }
  else if (neuron_type == UnitsTech::RESOURCENEURON) {
    spdlog::get(LOGGER)->debug("Player::AddNeuron: ResourceNeuron");
    std::string symbol = field_->GetSymbolAtPos(pos);
    int resource_type = symbol_resource_mapping.at(symbol);
    neurons_[pos] = std::make_shared<ResourceNeuron>(pos, resource_type);
  }
  else if (neuron_type == UnitsTech::LOOPHOLE) {
    spdlog::get(LOGGER)->debug("Player::AddNeuron: Loophole");
    position_t target = {-1, -1};
    // Get all loopholes.
    auto loopholes = GetAllPositionsOfNeurons(LOOPHOLE);
    // If one exists, this is the target.
    if (loopholes.size() == 1)
      target = loopholes.front();
    // If to exists, delete oldest, the other loophole is the new target.
    else if (loopholes.size() == 2) {
      if (utils::GetElapsedNano(neurons_.at(loopholes[0])->created_at(), neurons_.at(loopholes[1])->created_at()) > 0) {
        neurons_.erase(loopholes[0]);
        DeleteNeuron(LOOPHOLE, loopholes[0]);
        target = loopholes[1];
      }
      else {
        neurons_.erase(loopholes[1]);
        DeleteNeuron(LOOPHOLE, loopholes[1]);
        target = loopholes[0];
      }
    }
    // Create new loophole and make it the target of old loophole.
    neurons_[pos] = std::make_shared<Loophole>(pos, target);
    if (target.first != -1 && target.second != -1)
      neurons_[target]->set_target(pos);
  }
  CreateNeuron(neuron_type, pos);
  spdlog::get(LOGGER)->debug("Player::AddNeuron: done");
  return true;
}

bool Player::AddPotential(position_t synapes_pos, int unit, int inital_speed_decrease) {
  spdlog::get(LOGGER)->debug("Player::AddPotential.");
  if (!TakeResources(unit, false))
    return false;
  // Get way and target:
  // Check if synapses is blocked.
  if (neurons_.count(synapes_pos) == 0 || neurons_.at(synapes_pos)->type_ != SYNAPSE 
      || neurons_.at(synapes_pos)->blocked()) 
    return true;
  
  // Create way.
  auto way = field_->GetWay(synapes_pos, neurons_.at(synapes_pos)->GetWayPoints(unit));

  // Get boost from technologies.
  int potential_boast = technologies_.at(UnitsTech::ATK_POTENIAL).first;
  int speed_boast = technologies_.at(UnitsTech::ATK_POTENIAL).first;
  int duration_boast = technologies_.at(UnitsTech::ATK_DURATION).first;
  std::string id = "";
  if (unit == UnitsTech::EPSP) {
    // Increase num of currently stored epsps and get number of epsps to create.
    size_t num_epsps_to_create = neurons_.at(synapes_pos)->AddEpsp();
    spdlog::get(LOGGER)->debug("Player::AddPotential: epsp - creating {} epsps.", num_epsps_to_create);
    for (size_t i=0; i<num_epsps_to_create; i++) {
      id = utils::CreateId("epsp");
      potential_[id] = Epsp(synapes_pos, way, potential_boast, speed_boast);
    }
  }
  else if (unit == UnitsTech::IPSP) {
    spdlog::get(LOGGER)->debug("Player::AddPotential: ipsp - creating 1 ipsp.");
    id = utils::CreateId("ipsp");
    potential_[id] = Ipsp(synapes_pos, way, potential_boast, speed_boast, duration_boast);
  }
  else if (unit == UnitsTech::MACRO) {
    spdlog::get(LOGGER)->debug("Player::AddPotential: ipsp - creating 1 ipsp.");
    id = utils::CreateId("macro_" + std::to_string(macro_));
    potential_[id] = Makro(synapes_pos, way, potential_boast, 5*macro_+speed_boast);
  }
  // Decrease speed if given.
  potential_[id]._next_action += inital_speed_decrease;
  spdlog::get(LOGGER)->debug("Player::AddPotential: done.");
  statistics_->AddNewPotential(unit);
  return true;
}

bool Player::AddTechnology(int technology) {
  spdlog::get(LOGGER)->debug("Player::AddTechnology.");
  // Check if technology exists, resources are missing and whether already fully researched.
  if (technologies_.count(technology) == 0) {
    spdlog::get(LOGGER)->warn("Player::AddTechnology: technology does not exist");
    return false;
  }
  auto missing = GetMissingResources(technology, technologies_[technology].first+1);
  if (missing.size() > 0 || technologies_[technology].first == technologies_[technology].second) {
    spdlog::get(LOGGER)->debug("Player::AddTechnology: Not enough resources:");
    return false;
  }
  if (!TakeResources(technology, false, technologies_[technology].first+1)) {
    spdlog::get(LOGGER)->warn("Player::AddTechnology: taking resources failed.");
    return false;
  }
  // Handle technology.
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
  else if (technology == UnitsTech::DEF_SPEED) {
    for (auto& it : neurons_)
      if (it.second->type_ == UnitsTech::SYNAPSE)
        it.second->DecreaseTotalCooldown();
  }
  else if (technology == UnitsTech::TOTAL_RESOURCE) {
    UpdateResourceLimits(0.2);
  }
  else if (technology == UnitsTech::CURVE) {
    if (technologies_[technology].first == 3)
      resource_slowdown_ = 0.5;
    else 
      resource_slowdown_--;
  }
  else if (technology == UnitsTech::NUCLEUS_RANGE)
    cur_range_++;
  return true;
}

bool Player::DistributeIron(int resource) {
  spdlog::get(LOGGER)->debug("Player::DistributeIron: resource={}", resource);
  if (resources_.count(resource) == 0 || resource == IRON) {
    spdlog::get(LOGGER)->error("Player::DistributeIron: invalid resource!");
    return false;
  }
  else if (resources_.at(IRON).cur() < 1) {
    spdlog::get(LOGGER)->debug("Player::DistributeIron: not enough iron!");
    return false;
  }
  resources_.at(resource).set_distributed_iron(resources_.at(resource).distributed_iron()+1);
  // if it is now two, then resource is now active and we need to create a new resource-neuron.
  if (resources_.at(resource).distributed_iron() == 2)
    AddNeuron(resources_.at(resource).pos(), RESOURCENEURON);  // Add resource neuron.
  resources_.at(IRON).set_cur(resources_.at(IRON).cur() - 1);
  resources_.at(IRON).set_bound(resources_.at(IRON).bound() + 1);
  spdlog::get(LOGGER)->debug("Player::DistributeIron: success!");
  return true;
}

bool Player::RemoveIron(int resource) {
  spdlog::get(LOGGER)->debug("Player::RemoveIron: resource={}", resource);
  if (resources_.count(resource) == 0 || resource == IRON) {
    spdlog::get(LOGGER)->error("Player::RemoveIron: invalid resource!");
    return false;
  }
  if (resources_.at(resource).distributed_iron() == 0) {
    spdlog::get(LOGGER)->error("Player::RemoveIron: no iron distributed to this resource!");
    return false;
  }
  resources_.at(resource).set_distributed_iron(resources_.at(resource).distributed_iron()-1);
  if (resources_.at(resource).distributed_iron() == 1) {
    AddVoltageToNeuron(resources_.at(resource).pos(), 100);  // Remove resource neuron.
    // Add the second iron too.
    resources_.at(IRON).set_cur(resources_.at(IRON).cur() + 1);
  }
  resources_.at(IRON).set_cur(resources_.at(IRON).cur() + 1);
  resources_.at(IRON).set_bound(resources_.at(IRON).bound() -1);
  spdlog::get(LOGGER)->debug("Player::RemoveIron: success!");
  return true;
}

int Player::AddWayPosForSynapse(position_t synapse_pos, position_t way_position, bool reset) {
  spdlog::get(LOGGER)->debug("Player::AddWayPosForSynapse. synapse: {}, target: {}", 
      utils::PositionToString(synapse_pos), utils::PositionToString(way_position));
  if (technologies_.at(WAY).first < 1) {
    spdlog::get(LOGGER)->debug("Player::AddWayPosForSynapse. Not researched.");
    return -1;
  }
  if (!field_->graph()->InGraph(way_position)) {
    spdlog::get(LOGGER)->warn("Player::AddWayPosForSynapse. Target not in graph: {}", 
        utils::PositionToString(way_position));
    return -1;
  }
  if (neurons_.count(synapse_pos) && neurons_.at(synapse_pos)->type_ == UnitsTech::SYNAPSE) {
    auto cur_waypoints = neurons_.at(synapse_pos)->waypoints();
    if (reset)
      cur_waypoints.clear();
    cur_waypoints.push_back(way_position);
    neurons_.at(synapse_pos)->set_way_points(cur_waypoints);
    return cur_waypoints.size();
  }
  else {
    spdlog::get(LOGGER)->warn("Player::AddWayPosForSynapse: neuron not found or wrong type");
    return -1;
  }
}

bool Player::SwitchSwarmAttack(position_t pos) {
  if (neurons_.count(pos) && neurons_.at(pos)->type_ == UnitsTech::SYNAPSE) {
    neurons_.at(pos)->set_swarm(!neurons_.at(pos)->swarm());
    return neurons_.at(pos)->swarm();
  }
  return false;
}

void Player::ChangeIpspTargetForSynapse(position_t pos, position_t target_pos) {
  spdlog::get(LOGGER)->debug("Player::ChangeIpspTargetForSynapse: synapse: {}, target: {}", 
      utils::PositionToString(pos), utils::PositionToString(target_pos));
  if (neurons_.count(pos) && neurons_.at(pos)->type_ == UnitsTech::SYNAPSE)
    neurons_.at(pos)->set_ipsp_target_pos(target_pos);
}

void Player::ChangeEpspTargetForSynapse(position_t pos, position_t target_pos) {
  spdlog::get(LOGGER)->debug("Player::ChangeEpspTargetForSynapse: synapse: {}, target: {}", 
      utils::PositionToString(pos), utils::PositionToString(target_pos));
  if (neurons_.count(pos) && neurons_.at(pos)->type_ == UnitsTech::SYNAPSE)
    neurons_.at(pos)->set_epsp_target_pos(target_pos);
}

void Player::ChangeMacroTargetForSynapse(position_t pos, position_t target_pos) {
  spdlog::get(LOGGER)->debug("Player::ChangeMacroTargetForSynapse");
  if (neurons_.count(pos) && neurons_.at(pos)->type_ == UnitsTech::SYNAPSE)
    neurons_.at(pos)->set_macro_target_pos(target_pos);
}

void Player::UpdateStatisticsGraph() {
  statistics_->AddStatisticsEntry(resources_.at(OXYGEN).cur(), resources_.at(POTASSIUM).cur(), 
      resources_.at(CHLORIDE).cur(), resources_.at(GLUTAMATE).cur(), resources_.at(DOPAMINE).cur(),
      resources_.at(SEROTONIN).cur());
}


void Player::MovePotential() {
  spdlog::get(LOGGER)->debug("Player::MovePotential {}", potential_.size());
  // Move soldiers along the way to it's target and check if target is reached.
  std::vector<std::string> potential_to_remove;
  for (auto& it : potential_) {
    it.second._next_action--; // update cooldown.
    // If target not yet reached and it is time for the next action, move potential
    if (it.second._way.size() > 0 && it.second._next_action == 0) {
      // Move potential.
      it.second.pos_ = it.second._way.front(); 
      it.second._way.pop_front();
      it.second._next_action = it.second._speed; // potential did action, so reset speed.
      CheckLoophole(it.second);
    }
    // If target is reached, handle epsp and ipsp seperatly.
    // Epsp: add potential to target and add epsp to list of potentials to remove.
    if (it.second.type_ == UnitsTech::EPSP || it.second.type_ == UnitsTech::MACRO) {
      if (it.second._way.size() == 0) {
        HandleEpspAndMacro(it.second);
        potential_to_remove.push_back(it.first); // remove 
      }
    }
    // Ipsp: check if just time is up -> remove ipsp, otherwise -> block target.
    else {
      if (HandleIpsp(it.second, it.first)) {
        potential_to_remove.push_back(it.first);
      }
    }
  }
  // Remove potential which has reached it's target.
  for (const auto& it : potential_to_remove)
    potential_.erase(it);
  spdlog::get(LOGGER)->debug("Player::MovePotential done");
}

void Player::CheckLoophole(Potential& potential) {
  // loopholes only for ipsp and epsp
  if (!(potential.type_ == MACRO && macro_ == 1)) {
    // Check if entered own loophole:
    position_t loophole_target = GetLoopholeTargetIfExists(potential.pos_);
    // Check if entered enemy's loophole (if loophole is active):
    for (const auto& enemy : enemies_) {
      auto new_loophole_target = enemy->GetLoopholeTargetIfExists(potential.pos_, true);
      if (new_loophole_target.first != -1 && new_loophole_target.second != -1) {
        loophole_target = new_loophole_target;
        break;
      }
    }
    // If loophole-target exists, enter loophole.
    if (loophole_target.first != -1 && loophole_target.second != -1) {
      potential.pos_ = loophole_target;
      potential._way = field_->GetWay(potential.pos_, {potential._way.back()});
      potential._way.pop_front(); // remove first position, which is the loophole.
    }
  }
}

void Player::HandleEpspAndMacro(Potential& potential) {
  spdlog::get(LOGGER)->debug("Player::HandleEpspAndMacro: type: {}", potential.type_);
  // Epsp
  if (potential.type_ == UnitsTech::EPSP) {
    auto res = field_->GetNeuronTypeAndPlayerAtPosition(potential.pos_);
    if (res.first != -1)
      res.second->AddVoltageToNeuron(potential.pos_, potential._voltage);
  }
  // Makro (A)
  else if (potential.type_ == UnitsTech::MACRO && macro_ == 0) {
    auto res = field_->GetNeuronTypeAndPlayerAtPosition(potential.pos_);
    if (res.first != -1) {
      // Get all neurons in range of macro-position (range is matched with enemies current range)
      auto neurons = res.second->GetAllNeuronsInRange(potential.pos_);
      // Sort neurons by least distance to macro-position.
      std::map<double, FieldPosition> sorted_neurons;
      for (const auto& it : neurons)
        sorted_neurons.insert({utils::Dist(potential.pos_, it.pos()), it});
      int macro_voltage = potential._voltage;
      // Unload voltage on surounding enemy neuerons. (destroy enemy neurons, unit macro-voltage is 0)
      for (const auto& [_, neuron] : sorted_neurons) {
        int enemy_neuron_voltage = (neuron.unit() == SYNAPSE) ? 5 : (neuron.unit() == ACTIVATEDNEURON) ? 17 : 0;
        macro_voltage -= enemy_neuron_voltage;
        if (macro_voltage < 0) 
          enemy_neuron_voltage = macro_voltage+enemy_neuron_voltage;
        res.second->AddVoltageToNeuron(neuron.pos(), enemy_neuron_voltage);
        if (macro_voltage < 0) 
          break;
      }
    }
  }
  // Makro (B)
  else if (potential.type_ == UnitsTech::MACRO && macro_ == 1) {
    AddNeuron(potential.pos_, LOOPHOLE);
  }
}

bool Player::HandleIpsp(Potential& ipsp, std::string id) {
  spdlog::get(LOGGER)->debug("Player::HandleIpsp. pos: {}", utils::PositionToString(ipsp.pos_));
  // Potential swallow (i: 
  if (field_->epsps().count(ipsp.pos_) > 0) {
    // Go through all epsps at ipsp's position and check if belonging to other player (different color)
    std::vector<std::pair<std::string, std::shared_ptr<Player>>> vec = field_->epsps().at(ipsp.pos_);
    for (auto it = vec.begin(); it != vec.end(); it++) {
      // If so, neutralize potential
      if (it->second->color() != color_) {
        // Decrease enemy-potential by one. If destroyed, update statistics.
        if (it->second->NeutralizePotential(it->first, 1)) 
          statistics_->AddEpspSwallowed(); 
        // Increase potential by one (without destroying! If destroyed, return true, so ipsp is destroyed in loop).
        if (NeutralizePotential(id, -1, false))
          return true;
        break;
      }
    }
  }
  // If duration since last action is reached, add ipsp to list of potentials to remove and unblock target.
  if (ipsp._next_action == 0) {
    auto res = field_->GetNeuronTypeAndPlayerAtPosition(ipsp.pos_);
    if (res.first != -1)
      res.second->SetBlockForNeuron(ipsp.pos_, false);  // unblock target.
    return true;
  }
  // If target was reached for the first time, block target and change
  // movement form speed to duration.
  else if (ipsp._way.size() == 0 && !ipsp._target_blocked) {
    ipsp._next_action = ipsp._duration;
    ipsp._target_blocked = true;
    auto res = field_->GetNeuronTypeAndPlayerAtPosition(ipsp.pos_);
    if (res.first != -1)
      res.second->SetBlockForNeuron(ipsp.pos_, true);  // block target
  }
  return false;
}

void Player::HandleDef() {
  spdlog::get(LOGGER)->debug("Player::HandleDef");
  for (auto& neuron : neurons_) {
    if (neuron.second->type_ != UnitsTech::ACTIVATEDNEURON)
      continue;
    // Check if activated neurons recharge is done.
    if (neuron.second->DecreaseCooldown() && !neuron.second->blocked()) {
      // for every enemy
      for (const auto& enemy : enemies_) {
        // Check for potentials in range of activated neuron.
        for (const auto& potential : enemy->potential()) {
          if (utils::Dist(neuron.first, potential.second.pos_) < 3) {
            if (enemy->NeutralizePotential(potential.first, neuron.second->potential_slowdown()))
              statistics_->AddKilledPotential(potential.first);
            neuron.second->ResetMovement();  // neuron did action, so update last_action_.
            break;
          }
        }
      }
    }
  }
  spdlog::get(LOGGER)->debug("Player::HandleDef. Done");
}

void Player::IncreaseResources(bool inc_iron) {
  spdlog::get(LOGGER)->debug("Player::IncreaseResources, iron: {}, {}", resources_.at(IRON).cur(), 
      resources_.at(IRON).bound());
  double gain = std::abs(log(resources_.at(Resources::OXYGEN).cur()+0.5));
  for (auto& it : resources_) {
    it.second.call();
    // Inc only if min 2 iron is distributed, inc iron only depending on audio.
    if (it.first == IRON && inc_iron && it.second.cur()+it.second.bound()<it.second.limit())
      it.second.set_cur(it.second.cur()+1);
    else if (it.first != IRON && it.second.Active() && !it.second.blocked()) {
      it.second.Increase(gain, resource_slowdown_);
    }
  }
}

bool Player::HasLost() const {
  return lost_ || GetAllPositionsOfNeurons(NUCLEUS).size() == 0;
}

void Player::SetBlockForNeuron(position_t pos, bool blocked) {
  if (neurons_.count(pos) > 0) {
    neurons_[pos]->set_blocked(blocked);
    // If resource neuron, block/ unblock resource.
    if (neurons_[pos]->type_ == RESOURCENEURON)
      resources_.at(neurons_[pos]->resource()).set_blocked(blocked);
  }
}

bool Player::NeutralizePotential(std::string id, int voltage, bool erase) {
  spdlog::get(LOGGER)->debug("Player::NeutralizePotential: {} {}", id, voltage);
  if (potential_.count(id) > 0) {
    potential_.at(id)._voltage -= voltage;
    potential_.at(id)._hit = true;
    // Remove potential only if not already at it's target (length of way is greater than zero).
    if ((potential_.at(id)._voltage == 0 || potential_.at(id)._voltage > 10) && potential_.at(id)._way.size() > 0) {
      spdlog::get(LOGGER)->debug("Player::NeutralizePotential: potential: died.");
      if (erase)
        potential_.erase(id);
      statistics_->AddLostPotential(id);
      return true;
    }
  }
  return false;
}

void Player::AddVoltageToNeuron(position_t pos, int potential) {
  spdlog::get(LOGGER)->debug("Player::AddVoltageToNeuron: {} {}", utils::PositionToString(pos), potential);
  // If potential is negative: stop.
  if (potential < 0) {
    spdlog::get(LOGGER)->warn("Player::AddVoltageToNeuron: negative potential!");
    return;
  }
  // Check if neuron at given possition exists
  if (neurons_.count(pos) > 0) {
    if (neurons_.at(pos)->IncreaseVoltage(potential)) {
      int type = neurons_.at(pos)->type_;
      // If resource neuron was destroyed, remove all distributed iron.
      if (type == UnitsTech::RESOURCENEURON)
        resources_.at(neurons_.at(pos)->resource()).set_distributed_iron(0);
      // Remove and free resources.
      neurons_.erase(pos);
      FreeBoundResources(type);
      DeleteNeuron(type, pos);
      // Potentially deactivate all neurons formally in range of the destroyed nucleus.
      if (type == UnitsTech::NUCLEUS) {
        spdlog::get(LOGGER)->debug("Player::AddVoltageToNeuron: nucleus died!");
        CheckNeuronsAfterNucleusDies();
        UpdateResourceLimits(-0.1);  // Remove added max resources when nucleus dies.
      }
    }
  }
}

void Player::CheckNeuronsAfterNucleusDies() {
  spdlog::get(LOGGER)->debug("Player::CheckNeuronsAfterNucleusDies");
  // Get all nucleus and initialize vector of neurons to remove (with position and type)
  std::vector<position_t> all_nucleus = GetAllPositionsOfNeurons(UnitsTech::NUCLEUS);
  std::vector<std::pair<position_t, int>> neurons_to_remove;  // position and type
  // Gather all neurons no longer in range of a nucleus.
  for (const auto& neuron : neurons_) {
    // Nucleus are not affekted.
    if (neuron.second->type_ == UnitsTech::NUCLEUS)
      continue;
    // Add first and if in range of a nucleus, remove again.
    neurons_to_remove.push_back({neuron.first, neuron.second->type_});
    for (const auto& nucleus_pos : all_nucleus)
      if (utils::Dist(neuron.first, nucleus_pos) <= cur_range_) {
        neurons_to_remove.pop_back();
        break;
      }
  }
  // Remove neuerons no longer in range of nucleus, free bound resources and add to new-dead-neurons.
  for (const auto& it : neurons_to_remove) {
    neurons_.erase(it.first);
    FreeBoundResources(it.second);
    DeleteNeuron(it.second, it.first);
  }
}

std::vector<bool> Player::GetBuildingOptions() const{
  bool atleast_one_synapse = GetAllPositionsOfNeurons(SYNAPSE).size() > 0;
  return {
    (atleast_one_synapse && GetMissingResources(IPSP).size() == 0),
    (atleast_one_synapse && GetMissingResources(EPSP).size() == 0),
    (atleast_one_synapse && GetMissingResources(MACRO).size() == 0),
    (GetMissingResources(ACTIVATEDNEURON).size() == 0),
    (GetMissingResources(SYNAPSE).size() == 0),
    (GetMissingResources(NUCLEUS).size() == 0),
    (GetAllPositionsOfNeurons(SYNAPSE).size() > 0)
  };
}

std::vector<bool> Player::GetSynapseOptions() const {
  return {
    (technologies_.at(UnitsTech::WAY).first > 0),
    true,  // as this option is always availible (epsp-target)
    true,  // as this option is always availible (ipsp-target)
    true,  // as this option is always availible (macro-target)
    (technologies_.at(UnitsTech::SWARM).first > 0),
  };
}

void Player::UpdateResourceLimits(float faktor) {
  for (auto& it : resources_)
    it.second.set_limit(it.second.limit() + it.second.limit()*faktor);
}

void Player::CreateNeuron(int type, position_t pos) {
  new_neurons_[pos] = type;
  statistics_->AddNewNeuron(type);
  field_->AddNewNeuron(pos, neurons_.at(pos), shared_from_this());
  neuron_positions_[type].push_back(pos);
  neuron_positions_[-1].push_back(pos);
}

void Player::DeleteNeuron(int type, position_t pos) {
  spdlog::get(LOGGER)->debug("Player::DeleteNeuron: type: {}, pos: {}", type, utils::PositionToString(pos));
  new_dead_neurons_[pos] = type;
  field_->RemoveNeuron(pos);
  neuron_positions_[-1].erase(std::remove(neuron_positions_[-1].begin(), 
        neuron_positions_[-1].end(), pos), neuron_positions_[-1].end());
  neuron_positions_[type].erase(std::remove(neuron_positions_[type].begin(), 
        neuron_positions_[type].end(), pos), neuron_positions_[type].end());
}

position_t Player::GetRandomNeuron() {
  // Get all positions at which there are activated neurons
  std::vector<position_t> neuron_positions = GetAllPositionsOfNeurons(ACTIVATEDNEURON);
  // If no activated neurons, get all.
  if (neuron_positions.size() == 0)
    neuron_positions = GetAllPositionsOfNeurons();
  // If still no neurins, issue error and return DEFAULT_POS
  if (neuron_positions.size() == 0) {
    spdlog::get(LOGGER)->error("Player::GetRandomNeuron: no neurons!");
    return DEFAULT_POS;
  }
  // Otherwise, get random index and return position at index.
  int ran = ran_gen_->RandomInt(0, neuron_positions.size()-1);
  return neuron_positions[ran];
}

position_t Player::GetLoopholeTargetIfExists(position_t pos, bool only_active) const {
  if (only_active && GetMacroAtPosition().size() == 0)
    return DEFAULT_POS;
  if (neurons_.count(pos) && neurons_.at(pos)->type_ == LOOPHOLE) 
    return neurons_.at(pos)->target();
  return DEFAULT_POS;
}
