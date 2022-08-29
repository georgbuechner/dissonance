#include "share/objects/units.h"
#include "share/constants/codes.h"
#include "share/tools/utils/utils.h"
#include "spdlog/spdlog.h"
#include <cstddef>

// Neurons
Neuron::Neuron() : Unit () {}
Neuron::Neuron(position_t pos, int lp, int type) : Unit(pos, type) {
  lp_ = 0;
  max_lp_ = lp;
  blocked_ = false;
}

Neuron::Neuron(const Neuron& neuron) {
  pos_ = neuron.pos_;
  type_ = neuron.type_;
  lp_ = neuron.lp_;
  max_lp_ = neuron.max_lp_;
  blocked_ = neuron.blocked_;
}

// getter 
int Neuron::voltage() const { 
  return lp_; 
}
int Neuron::max_voltage() const { 
  return max_lp_; 
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
  macro_target_ = epsp_target;
  num_availible_way_points_ = num_availible_ways;
}

Synapse::Synapse(const Neuron& neuron) : Neuron(neuron) {
  swarm_ = neuron.swarm();
  max_stored_ = neuron.max_stored();
  stored_ = neuron.stored();
  epsp_target_ = neuron.ipsp_target();
  ipsp_target_ = neuron.epsp_target();
  macro_target_ = neuron.macro_target();
  num_availible_way_points_ = neuron.num_availible_ways();
  way_points_ = neuron.ways_points();
  spdlog::get(LOGGER)->debug("Added Synape. type: {}", type_);
}

// getter: 
std::vector<position_t> Synapse::ways_points() const { 
  return way_points_; 
}
bool Synapse::swarm() const { 
  return swarm_; 
}
unsigned int Synapse::num_availible_ways() const { 
  return num_availible_way_points_; 
}
unsigned int Synapse::max_stored() const { 
  return max_stored_; 
}
unsigned int Synapse::stored() const { 
  return stored_; 
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
void Synapse::set_macro_target_pos(position_t pos) {
  macro_target_ = pos;
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
  else if (unit == UnitsTech::MACRO) 
    way.push_back(macro_target_);
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

// Activated neurons...
ActivatedNeuron::ActivatedNeuron() : Neuron() {}
ActivatedNeuron::ActivatedNeuron(position_t pos, int slowdown_boast, int speed_boast) : 
    Neuron(pos, 17, UnitsTech::ACTIVATEDNEURON) {
  movement_ = {0, ACTIVATEDNEURON_SPEED-speed_boast};
  potential_slowdown_ = 1+slowdown_boast;
}

ActivatedNeuron::ActivatedNeuron(const Neuron& neuron) : Neuron(neuron) {
  movement_ = neuron.movement();
  potential_slowdown_ = neuron.potential_slowdown();
  spdlog::get(LOGGER)->debug("Added ActivatedNeuron. type: {}", type_);
}

// getter 
int ActivatedNeuron::cur_movement() const { 
  return movement_.first; 
}
std::pair<int, int> ActivatedNeuron::movement() const { 
  return movement_; 
}
int ActivatedNeuron::potential_slowdown() const { 
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
ResourceNeuron::ResourceNeuron(const Neuron& neuron) : Neuron(neuron), resource_(neuron.resource()) { 
  spdlog::get(LOGGER)->debug("Added ResourceNeuron. type: {}", type_);
}

// getter 
size_t ResourceNeuron::resource() const {
  return resource_;
}
