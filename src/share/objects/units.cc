#include "share/objects/units.h"
#include "share/constants/codes.h"
#include "share/defines.h"
#include "share/tools/utils/utils.h"

// Neurons
Neuron::Neuron() : Unit () {}
Neuron::Neuron(position_t pos, int lp, int type) : Unit(pos, type) {
  voltage_ = 0;
  max_voltage_ = lp;
  blocked_ = false;
}

// getter 
int Neuron::voltage() const { 
  return voltage_; 
}
int Neuron::max_voltage() const { 
  return max_voltage_; 
}
bool Neuron::blocked() const { return blocked_; };

// setter
void Neuron::set_blocked(bool blocked) { 
  blocked_ = blocked; 
}

// methods

/**
 * Increases voltage of neuron and returns whether neuron is destroyed.
 */
bool Neuron::IncreaseVoltage(int voltage) {
  if (voltage < 0)
    return false;
  voltage_ += voltage;
  return voltage_ >= max_voltage_;
}

// Synapses ...
Synapse::Synapse() : Neuron() {}
Synapse::Synapse(position_t pos, int max_stored, int num_availible_ways, position_t epsp_target, position_t ipsp_target) 
  : Neuron(pos, 5, UnitsTech::SYNAPSE) 
{
  swarm_ =false;
  max_stored_ = max_stored;
  stored_ = 0;
  epsp_target_ = epsp_target;
  ipsp_target_ = ipsp_target;
  macro_target_ = epsp_target;
  num_availible_waypoints_ = num_availible_ways;
}

// getter: 
std::vector<position_t> Synapse::waypoints() const { 
  return waypoints_; 
}
bool Synapse::swarm() const { 
  return swarm_; 
}
int Synapse::num_availible_ways() const { 
  return num_availible_waypoints_; 
}
int Synapse::max_stored() const { 
  return max_stored_; 
}
int Synapse::stored() const { 
  return stored_; 
}
position_t Synapse::epsp_target() const { 
  return epsp_target_; 
}
position_t Synapse::ipsp_target() const { 
  return ipsp_target_; 
}
position_t Synapse::macro_target() const { 
  return macro_target_; 
}
position_t Synapse::GetTargetForPotential(int unit)  const { 
  if (unit == IPSP) return ipsp_target_;
  if (unit == EPSP) return epsp_target_;
  if (unit == MACRO) return macro_target_;
  return DEFAULT_POS;
}

// setter: 
void Synapse::set_way_points(std::vector<position_t> way_points) { 
  waypoints_ = way_points; 
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
void Synapse::set_macro_target_pos(position_t pos) {
  macro_target_ = pos;
}
void Synapse::set_availible_ways(int num_availible_way_points) {
  num_availible_waypoints_ = num_availible_way_points;
}
void Synapse::set_max_stored(int max_stored) {
  max_stored_ = max_stored;
}

// methods: 
std::vector<position_t> Synapse::GetWayPoints(int unit) const { 
  auto waypoints_with_target = waypoints_;
  if (unit == UnitsTech::EPSP)
    waypoints_with_target.push_back(epsp_target_);
  else if (unit == UnitsTech::IPSP)
    waypoints_with_target.push_back(ipsp_target_);
  else if (unit == UnitsTech::MACRO) 
    waypoints_with_target.push_back(macro_target_);
  else
   throw std::invalid_argument("Target neither epsp or ipsp:");
  return waypoints_with_target;
}

int Synapse::AddEpsp() { 
  if (swarm_) {
    if (++stored_ >= max_stored_) {
      stored_ = 0;
      return max_stored_;
    }
    return 0;
  }
  return 1; 
};

// Activated neurons...
ActivatedNeuron::ActivatedNeuron() : Neuron() {}
ActivatedNeuron::ActivatedNeuron(position_t pos, int slowdown_boast, int speed_boast) : 
    Neuron(pos, 17, UnitsTech::ACTIVATEDNEURON) {
  next_action_ = 0;
  cooldown_ = ACTIVATEDNEURON_SPEED-speed_boast;
  potential_slowdown_ = 1+slowdown_boast;
}

// getter 
int ActivatedNeuron::potential_slowdown() const { 
  return potential_slowdown_; 
}

bool ActivatedNeuron::DecreaseCooldown() {
  // Make sure next_action_ is never negative.
  if (next_action_ > 0)
    next_action_--;
  return next_action_ == 0;
}

void ActivatedNeuron::ResetMovement() {
  next_action_ = cooldown_;
}

void ActivatedNeuron::DecreaseTotalCooldown() {
  cooldown_--;
}

// ResourceNeuron...
ResourceNeuron::ResourceNeuron() : Neuron(), resource_(999) {}
ResourceNeuron::ResourceNeuron(position_t pos, size_t resource) : Neuron(pos, 0, UnitsTech::RESOURCENEURON), 
    resource_(resource) {
}

// getter 
size_t ResourceNeuron::resource() const {
  return resource_;
}
