#ifndef SRC_SOLDIER_H_
#define SRC_SOLDIER_H_

#include <chrono>
#include <cstddef>
#include <iostream>
#include <list>
#include <stdexcept>
#include <vector>
#include <spdlog/spdlog.h>

#include "share/constants/codes.h"
#include "share/defines.h"

typedef std::pair<int, int> position_t;

/**
 * Abstrackt class for all neurons or potentials.
 * Attributes:
 * - pos
 */
struct Unit {
  position_t pos_;
  int type_;

  Unit() : pos_({}) {}
  Unit(position_t pos, int type) : pos_(pos), type_(type) {}
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
  public:
    // getter 
    int voltage();
    int max_voltage();
    bool blocked();
    virtual int speed() { return -1; };
    virtual int potential_slowdown() { return -1; };
    virtual std::chrono::time_point<std::chrono::steady_clock> last_action() {return std::chrono::steady_clock::now(); }
    virtual std::vector<position_t> ways_points() {return {}; }
    virtual bool swarm() { return false; }
    virtual unsigned int num_availible_ways() { return 0; }
    virtual unsigned int max_stored() { return 0; }
    virtual size_t resource() { return 9999; }
    virtual position_t target(int unit) { return {-1, -1}; }

    // setter
    void set_blocked(bool blocked);
    virtual void set_last_action(std::chrono::time_point<std::chrono::steady_clock> time) {};
    virtual void set_way_points(std::vector<position_t> pos) {};
    virtual void set_swarm(bool swarm) {};
    virtual void set_epsp_target_pos(position_t pos) {};
    virtual void set_ipsp_target_pos(position_t pos) {};
    virtual void set_availible_ways(unsigned int num_ways) {};
    virtual void set_max_stored(unsigned int max_stored) {};

    // methods

    /**
     * Increases voltage of neuron and returns whether neuron is destroyed.
     */
    bool IncreaseVoltage(int potential);

    virtual std::vector<position_t> GetWayPoints(int unit) const { 
      spdlog::get(LOGGER)->error("Neuron::GetWayPoints: invalid base clase call!");
      return {}; 
    }
    virtual unsigned int AddEpsp() { return 0; }
    virtual void UpdateIpspTargetIfNotSet(position_t pos) { }

    Neuron();
    Neuron(position_t pos, int lp, int type);
    virtual ~Neuron() {}

  private:
    int lp_;
    int max_lp_;
    bool blocked_;
};

/** 
 * Implemented class: Synapse.
 * Synapse is a special kind of neuron, which can create potential.
 * Attributes:
 * - pos (derived from Unit)
 * - lp (derived from Neuron)
 */
struct Synapse : Neuron {
  public: 
    Synapse();
    Synapse(position_t pos, int max_stored, int num_availible_ways, position_t epsp_target, position_t ipsp_target);

    // getter: 
    std::vector<position_t> ways_points();
    bool swarm();
    unsigned int num_availible_ways();
    unsigned int max_stored();
    position_t target(int unit) { 
      if (unit == IPSP) return ipsp_target_;
      if (unit == EPSP) return epsp_target_;
      return {-1, -1};
    }
   
    // setter: 
    void set_way_points(std::vector<position_t> way_points);
    void set_swarm(bool swarm);
    void set_epsp_target_pos(position_t pos);
    void set_ipsp_target_pos(position_t pos);
    void set_availible_ways(unsigned int num_availible_way_points);
    void set_max_stored(unsigned int max_stored);

    // methods: 
    std::vector<position_t> GetWayPoints(int unit) const;
    unsigned int AddEpsp();
    void UpdateIpspTargetIfNotSet(position_t pos);

  private:
    bool swarm_;
    int max_stored_;
    int stored_;
    
    position_t epsp_target_;
    position_t ipsp_target_;

    unsigned int num_availible_way_points_;
    std::vector<position_t> way_points_;
};

/** 
 * Implemented class: ActivatedNeuron.
 * Activated neurons are a defence mechanism agains incoming potential.
 * Attributes:
 * - pos (derived from Unit)
 * - lp (derived from Neuron)
 */
struct ActivatedNeuron : Neuron {
  public:
    ActivatedNeuron();
    ActivatedNeuron(position_t pos, int slowdown_boast, int speed_boast);

    // getter 
    int speed();
    int potential_slowdown();
    std::chrono::time_point<std::chrono::steady_clock> last_action();
    
    // setter
    void set_last_action(std::chrono::time_point<std::chrono::steady_clock> time);

  private:
    int speed_;  ///< lower number means higher speed.
    int potential_slowdown_;
    std::chrono::time_point<std::chrono::steady_clock> last_action_; 
};

/**
 *
 */
struct ResourceNeuron : Neuron {
  public: 
    ResourceNeuron();
    ResourceNeuron(position_t, size_t resource);

    // getter 
    size_t resource();

  private:
    const size_t resource_;
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
  Nucleus(position_t pos) : Neuron(pos, 9, UnitsTech::NUCLEUS) {}
};

/**
 * Abstrackt class for all potentials.
 * Attributes:
 * - pos (derived from Unit)
 */
struct Potential : Unit {
  int potential_;
  int speed_;  ///< lower number means higher speed.
  int duration_; ///< only potential
  std::chrono::time_point<std::chrono::steady_clock> last_action_; 
  std::list<position_t> way_;

  Potential() : Unit(), speed_(999), last_action_(std::chrono::steady_clock::now()) {}
  Potential(position_t pos, int attack, std::list<position_t> way, int speed, int type, int duration) 
    : Unit(pos, type), potential_(attack), speed_(speed), duration_(duration),
    last_action_(std::chrono::steady_clock::now()), way_(way) {}
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
  Epsp(position_t pos, std::list<position_t> way, int potential_boast, int speed_boast) 
    : Potential(pos, 2+potential_boast, way, 370-speed_boast, UnitsTech::EPSP, 0) {}
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

  Ipsp() : Potential() {}
  Ipsp(position_t pos, std::list<position_t> way, int potential_boast, int speed_boast, int duration_boast) 
    : Potential(pos, 3+potential_boast, way, 420-speed_boast, UnitsTech::IPSP, 4+duration_boast) {}
};

#endif
