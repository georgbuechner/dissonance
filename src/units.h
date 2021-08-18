#ifndef SRC_SOLDIER_H_
#define SRC_SOLDIER_H_

#include <chrono>
#include <list>

typedef std::pair<int, int> Position;

/**
 * Abstrackt class for all neurons or potentials.
 * Attributes:
 * - pos
 */
struct Unit {
  Position pos_;

  Unit() : pos_({}) {}
  Unit(Position pos) : pos_(pos) {}
};

/**
 * Abstrackt class for all neurons.
 * Neurons are non-moving units on the field (buildings), which have lp. When
 * lp<=0, a neuron is destroyed
 * Attributes:
 * - pos (derived from Unit)
 * - lp 
 */
struct Neuron : Unit {
  int lp_;
  int max_lp_;
  Neuron() : Unit() {}
  Neuron(Position pos, int lp) : Unit(pos), lp_(0), max_lp_(lp) {}
};

/** 
 * Implemented class: Synapse.
 * Synapse is a special kind of neuron, which can create potential.
 * Attributes:
 * - pos (derived from Unit)
 * - lp (derived from Neuron)
 */
struct Synapse : Neuron {
  int stored_;
  int target_;

  Synapse() : Neuron() {}
  Synapse(Position pos) : Neuron(pos, 5) {}
};

/** 
 * Implemented class: ActivatedNeuron.
 * Activated neurons are a defence mechanism agains incoming potential.
 * Attributes:
 * - pos (derived from Unit)
 * - lp (derived from Neuron)
 */
struct ActivatedNeuron : Neuron {
  int speed_;  ///< lower number means higher speed.
  std::chrono::time_point<std::chrono::steady_clock> last_action_; 

  ActivatedNeuron() : Neuron() {}
  ActivatedNeuron(Position pos) : Neuron(pos, 17), speed_(700), last_action_(std::chrono::steady_clock::now()) {}
};

/** 
 * Implemented class: Synapse.
 * The Nucleus is the main point from which your system is developing.
 * Attributes:
 * - pos (derived from Unit)
 * - lp (derived from Neuron)
 */
struct Nucleus : Neuron {
  Nucleus() : Neuron() {}
  Nucleus(Position pos) : Neuron(pos, 9) {}
};

/**
 * Abstrackt class for all potentials.
 * Attributes:
 * - pos (derived from Unit)
 */
struct Potential : Unit {
  int attack_;
  int speed_;  ///< lower number means higher speed.
  std::chrono::time_point<std::chrono::steady_clock> last_action_; 
  std::list<Position> way_;

  Potential() : Unit(), speed_(999), last_action_(std::chrono::steady_clock::now()) {}
  Potential(Position pos, int attack, std::list<Position> way, int speed) : Unit(pos), attack_(attack), speed_(speed), last_action_(std::chrono::steady_clock::now()), way_(way) {}
};

/**
 * Implemented class: EPSP
 * Attributes:
 * - pos (derived from Unit)
 * - attack (derived from Potential)
 * - speed (derived from Potential)
 * - last_action (derived from Potential)
 * - way (derived from Potential)
 */
struct Epsp : Potential {
  Epsp() : Potential() {}
  Epsp(Position pos, std::list<Position> way) : Potential(pos, 2, way, 370) {}
};

/**
 * Implemented class: IPSP.
 * Attributes:
 * - pos (derived from Unit)
 * - attack (derived from Potential)
 * - speed (derived from Potential)
 * - last_action (derived from Potential)
 * - way (derived from Potential)
 */
struct Ipsp: Potential {
  int duration_;

  Ipsp() : Potential() {}
  Ipsp(Position pos, std::list<Position> way, int duration) : Potential(pos, 1, way, 420), duration_(duration) {}
};

#endif
