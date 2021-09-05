#include "spdlog/spdlog.h"
#include <units.h>

// Neurons
Neuron::Neuron() : Unit() {}
Neuron::Neuron(Position pos, int lp, int type) : Unit(pos, type) {
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
  lp_ += potential;
  if (lp_ >= max_lp_) 
    return true;
  return false;
}

// Synapses ...
Synapse::Synapse() : Neuron() {}
Synapse::Synapse(Position pos, int max_stored, int num_availible_ways, Position epsp_target, Position ipsp_target) :
    Neuron(pos, 5, UnitsTech::SYNAPSE) {
  swarm_ =false;
  max_stored_ = max_stored;
  stored_ = 0;
  epsp_target_ = epsp_target;
  ipsp_target_ = ipsp_target;
  num_availible_way_points_ = num_availible_ways;
}

// getter: 
std::vector<Position> Synapse::ways_points() { 
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
void Synapse::set_way_points(std::vector<Position> way_points) { 
  way_points_ = way_points; 
}
void Synapse::set_swarm(bool swarm) {
  swarm_ = swarm;
}
void Synapse::set_epsp_target_pos(Position pos) {
  epsp_target_ = pos;
}
void Synapse::set_ipsp_target_pos(Position pos) {
  ipsp_target_ = pos;
}
void Synapse::set_availible_ways(unsigned int num_availible_way_points) {
  num_availible_way_points_ = num_availible_way_points;
}
void Synapse::set_max_stored(unsigned int max_stored) {
  max_stored_ = max_stored;
}

// methods: 
std::vector<Position> Synapse::GetWayWithTargetIncluded(int unit) { 
  spdlog::get(LOGGER)->debug("SYNAPSE::GetWayWithTargetIncluded");
  auto way = way_points_;
  if (unit == UnitsTech::EPSP)
    way.push_back(epsp_target_);
  else if (unit == UnitsTech::IPSP)
    way.push_back(epsp_target_);
  else
   throw std::invalid_argument("Target neither epsp or ipsp:");
  return way;
}
unsigned int Synapse::AddEpsp() { 
  spdlog::get(LOGGER)->debug("Synapse::AddEpsp");
  if (swarm_) {
    if (++stored_ >= max_stored_)
      return max_stored_;
    return 0;
  }
  return 1; 
};

// Activated neurons...
ActivatedNeuron::ActivatedNeuron() : Neuron() {}
ActivatedNeuron::ActivatedNeuron(Position pos, int slowdown_boast, int speed_boast) : 
    Neuron(pos, 17, UnitsTech::ACTIVATEDNEURON) {
  speed_ = 700-speed_boast;
  potential_slowdown_ = 1+slowdown_boast;
  last_action_ = std::chrono::steady_clock::now();
}

// getter 
int ActivatedNeuron::speed() { 
  return speed_; 
}
int ActivatedNeuron::potential_slowdown() { 
  return potential_slowdown_; 
}
std::chrono::time_point<std::chrono::steady_clock> ActivatedNeuron::last_action() { 
  return last_action_; 
}

// setter
void ActivatedNeuron::set_last_action(std::chrono::time_point<std::chrono::steady_clock> time) { 
  last_action_ = time; 
}
