#include "share/objects/units.h"
#include "share/tools/utils/utils.h"
#include <cstddef>

// Neurons
Neuron::Neuron() : Unit() {}
Neuron::Neuron(position_t pos, int lp, int type) : Unit(pos, type) {
  lp_ = 0;
  max_lp_ = lp;
  blocked_ = false;
}

// getter 
int Neuron::voltage() { 
  return lp_; 
}
int Neuron::max_voltage() { 
  return max_lp_; 
}
bool Neuron::blocked() { return blocked_; };

// setter
void Neuron::set_blocked(bool blocked) { 
  blocked_ = blocked; 
}

// methods

/**
 * Increases voltage of neuron and returns whether neuron is destroyed.
 */
bool Neuron::IncreaseVoltage(int potential) {
  if (potential < 0)
    return false;
  lp_ += potential;
  return (lp_ >= max_lp_) ? true : false;
}

// Synapses ...
Synapse::Synapse() : Neuron() {}
Synapse::Synapse(position_t pos, int max_stored, int num_availible_ways, position_t epsp_target, position_t ipsp_target) :
    Neuron(pos, 5, UnitsTech::SYNAPSE) {
  swarm_ =false;
  max_stored_ = max_stored;
  stored_ = 0;
  epsp_target_ = epsp_target;
  ipsp_target_ = ipsp_target;
  num_availible_way_points_ = num_availible_ways;
}

// getter: 
std::vector<position_t> Synapse::ways_points() { 
  return way_points_; 
}
bool Synapse::swarm() { 
  return swarm_; 
}
unsigned int Synapse::num_availible_ways() { 
  return num_availible_way_points_; 
}
unsigned int Synapse::max_stored() { 
  return max_stored_; 
}

// setter: 
void Synapse::set_way_points(std::vector<position_t> way_points) { 
  way_points_ = way_points; 
}
void Synapse::set_swarm(bool swarm) {
  swarm_ = swarm;
}
void Synapse::set_epsp_target_pos(position_t pos) {
  epsp_target_ = pos;
}
void Synapse::set_ipsp_target_pos(position_t pos) {
  ipsp_target_ = pos;
}
void Synapse::set_availible_ways(unsigned int num_availible_way_points) {
  num_availible_way_points_ = num_availible_way_points;
}
void Synapse::set_max_stored(unsigned int max_stored) {
  max_stored_ = max_stored;
}

// methods: 
std::vector<position_t> Synapse::GetWayPoints(int unit) const { 
  spdlog::get(LOGGER)->debug("SYNAPSE::GetWayPoints");
  auto way = way_points_;
  if (unit == UnitsTech::EPSP)
    way.push_back(epsp_target_);
  else if (unit == UnitsTech::IPSP)
    way.push_back(ipsp_target_);
  else
   throw std::invalid_argument("Target neither epsp or ipsp:");
  return way;
}

unsigned int Synapse::AddEpsp() { 
  spdlog::get(LOGGER)->debug("Synapse::AddEpsp");
  if (swarm_) {
    if (++stored_ >= max_stored_) {
      stored_ = 0;
      return max_stored_;
    }
    return 0;
  }
  return 1; 
};

void Synapse::UpdateIpspTargetIfNotSet(position_t pos) {
  if (ipsp_target_.first == -1) {
    ipsp_target_ = pos;
    spdlog::get(LOGGER)->info("Updated ipsp target to: {}", utils::PositionToString(pos));
  }
}

// Activated neurons...
ActivatedNeuron::ActivatedNeuron() : Neuron() {}
ActivatedNeuron::ActivatedNeuron(position_t pos, int slowdown_boast, int speed_boast) : 
    Neuron(pos, 17, UnitsTech::ACTIVATEDNEURON) {
  movement_ = {0, 5-speed_boast};
  potential_slowdown_ = 1+slowdown_boast;
}

// getter 
int ActivatedNeuron::movement() { 
  return movement_.first; 
}
int ActivatedNeuron::potential_slowdown() { 
  return potential_slowdown_; 
}

void ActivatedNeuron::decrease_cooldown() {
  if (movement_.first > 0)
    movement_.first--;
}
void ActivatedNeuron::reset_movement() {
  movement_.first = movement_.second;
}


// ResourceNeuron...
ResourceNeuron::ResourceNeuron() : Neuron(), resource_(999) {}
ResourceNeuron::ResourceNeuron(position_t pos, size_t resource) : Neuron(pos, 0, UnitsTech::RESOURCENEURON), 
    resource_(resource) {
  spdlog::get(LOGGER)->debug("ResourceNeuron::ResourceNeuron, type {}", UnitsTech::RESOURCENEURON);
}

// getter 
size_t ResourceNeuron::resource() {
  return resource_;
}
