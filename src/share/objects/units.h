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

#define EPSP_SPEED 6 
#define IPSP_SPEED 8 
#define MACRO_SPEED 10
#define ACTIVATEDNEURON_SPEED 6
#define IPSP_DURATION 30


/**
 * Abstract class for all neurons or potentials.
 * Attributes:
 */
struct Unit {
  position_t pos_;
  int type_;

  Unit() : pos_({}) {}
  Unit(position_t pos, int type) : pos_(pos), type_(type) {}
};

/**
 * Abstract class for all neurons.
 * Neurons are non-moving units on the field (buildings), which have voltage
 * When voltage>=max_voltage, a neuron is destroyed
 */
struct Neuron : Unit {
  public:
    Neuron();
    Neuron(position_t pos, int lp, int type);
    virtual ~Neuron() {}

    // getter 
    int voltage() const;
    int max_voltage() const;
    bool blocked() const;
    virtual int potential_slowdown() const { return -1; };
    virtual std::vector<position_t> waypoints() const { return {}; }
    virtual bool swarm() const { return false; }
    virtual int num_availible_ways() const { return 0; }
    virtual int max_stored() const { return 0; }
    virtual int stored() const { return 0; }
    virtual position_t epsp_target() const { return DEFAULT_POS; }
    virtual position_t ipsp_target() const { return DEFAULT_POS; }
    virtual position_t macro_target() const { return DEFAULT_POS; }
    virtual size_t resource() const { return 9999; }
    virtual position_t target() const { return {-1, -1}; }
    virtual std::chrono::time_point<std::chrono::steady_clock> created_at() const { 
      return std::chrono::steady_clock::now(); 
    }

    // setter
    void set_blocked(bool blocked);
    virtual void set_way_points(std::vector<position_t> pos) {};
    virtual void set_swarm(bool swarm) {};
    virtual void set_epsp_target_pos(position_t pos) {};
    virtual void set_ipsp_target_pos(position_t pos) {};
    virtual void set_macro_target_pos(position_t pos) {};
    virtual void set_availible_ways(int num_ways) {};
    virtual void set_max_stored(int max_stored) {};
    virtual void set_target(position_t) {};

    // methods
    virtual bool DecreaseCooldown() { return false; }
    virtual void ResetMovement() {}
    virtual void DecreaseTotalCooldown() {};

    virtual position_t GetTargetForPotential(int unit) const { return {-1, -1}; }
    virtual std::vector<position_t> GetWayPoints(int unit) const { return {}; }
    virtual int AddEpsp() { return 0; }

    /**
     * Increases voltage of neuron and returns whether neuron is destroyed.
     * @param[in] voltage 
     * @return whether neuron is destroyed (voltage exceeds max-voltage)
     */
    bool IncreaseVoltage(int voltage);

  private:
    int voltage_;
    int max_voltage_;
    bool blocked_;
};

/** 
 * Implemented class: Synapse.
 * Synapse is a special kind of neuron, which can create potentials.
 */
struct Synapse : Neuron {
  public: 
    Synapse();
    Synapse(position_t pos, int max_stored, int num_availible_ways, position_t epsp_target, position_t ipsp_target);

    // getter: 
    std::vector<position_t> waypoints() const;
    bool swarm() const;
    int num_availible_ways() const;
    int max_stored() const;
    int stored() const;
    position_t epsp_target() const;
    position_t ipsp_target() const;
    position_t macro_target() const;
   
    // setter: 
    void set_way_points(std::vector<position_t> way_points);
    void set_swarm(bool swarm);
    void set_epsp_target_pos(position_t pos);
    void set_ipsp_target_pos(position_t pos);
    void set_macro_target_pos(position_t pos);
    void set_availible_ways(int num_availible_way_points);
    void set_max_stored(int max_stored);

    // methods: 
    
    /**
     * Gets the target matching the given potential (EPSP, IPSP, MACRO).
     * @param[in] unit (must be EPSP, IPSP or MACRO)
     * @return position of matching target or DEFAULT_POS (if unit does not match EPSP, IPSP or MACRO.
     */
    position_t GetTargetForPotential(int unit) const;

    /**
     * Gets way-points of the synapse with the target matching the given unit
     * included as last waypoint.
     * Throws if unit does not match (EPSP, IPSP or MACRO)
     * @param[in] unit 
     * @return waypoints + `unit`'s target
     */
    std::vector<position_t> GetWayPoints(int unit) const;

    /**
     * Adds epsp to stored if `swarm_==true`, and returns number of epsps to
     * create. 
     * Either 1 (`!swarm_`) otherwise 0, or, if `stored_==max_stored_` is reached
     * `max_stored` (if this case `stored_` is reset).
     * @return number of epsps to create (1, 0, or max_stored_)
     */
    int AddEpsp();

  private:
    bool swarm_;
    int max_stored_;
    int stored_;
    
    position_t epsp_target_;
    position_t ipsp_target_;
    position_t macro_target_;

    int num_availible_waypoints_;
    std::vector<position_t> waypoints_;
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
    int potential_slowdown() const;
    
    /**
     * Decreases next-action by one returns true, when next_action_ is 0.
     * @return true if next_action_ is zero.
     */
    bool DecreaseCooldown();

    /**
     * Resets next_action_ to cooldown.
     */
    void ResetMovement();

    /**
     * Decreases cooldown.
     */
    void DecreaseTotalCooldown();

  private:
    int next_action_;  ///< when 0, does action (reduce incoming potentials voltage)
    int cooldown_; 
    int potential_slowdown_;
};

/**
 * Implemented class: ResourceNeuron.
 * Resource neurons automatically created when >2 iron is distributed to a
 * resource and destroyed when iron is withdrawn from resource. 
 * When resource-neuron is destroyed, all iron is withdrawn from resource.
 */
struct ResourceNeuron : Neuron {
  public: 
    ResourceNeuron();
    ResourceNeuron(position_t, size_t resource);

    // getter 
    size_t resource() const;

  private:
    const size_t resource_;
};

/** 
 * Implemented class: Nucleus.
 * The Nucleus is the main point from which your system is developing.
 */
struct Nucleus : Neuron {
  Nucleus() : Neuron() {}
  Nucleus(position_t pos) : Neuron(pos, 9, UnitsTech::NUCLEUS) {}
};

/** 
 * Implemented class: Loophole.
 * The Nucleus is the main point from which your system is developing.
 */
struct Loophole : Neuron {
  public: 
    Loophole() : Neuron(), target_({-1, -1}), created_at_(std::chrono::steady_clock::now()) {}
    Loophole(position_t pos, position_t target) : Neuron(pos, 9, UnitsTech::LOOPHOLE), target_(target) {}

    // getter
    position_t target() const { return target_; }
    std::chrono::time_point<std::chrono::steady_clock> created_at() const { return created_at_; }

    // setter
    void set_target(position_t pos) { target_ = pos; }

  private:
    position_t target_;
    const std::chrono::time_point<std::chrono::steady_clock> created_at_;
};

/**
 * Abstract class for all potentials.
 */
struct Potential : Unit {
  int _voltage;  ///< voltage added to neuron when hit.
  int _next_action;  ///< indicates when next action will happen 
  int _speed;  ///< initial value for `next_action_`. `next_action_` reset to speed after action.
  int _duration; ///< only ipsp: duration for blocking
  std::list<position_t> _way; 
  bool _target_blocked;  ///< only ipsp: indicates whether ipsp is currently blocking it's target.

  Potential() : Unit(), _voltage(0), _next_action(999), _speed(999), _duration(0) {}
  Potential(position_t pos, int attack, std::list<position_t> way, int speed, int type, int duration) 
    : Unit(pos, type), _voltage(attack), _next_action(speed), _speed(speed), _duration(duration), 
    _way(way), _target_blocked(false) {}
};

/**
 * Implemented class: EPSP
 */
struct Epsp : Potential {
  Epsp() : Potential() {}
  Epsp(position_t pos, std::list<position_t> way, int potential_boast, int speed_boast) 
    : Potential(pos, 2+potential_boast, way, EPSP_SPEED-speed_boast, UnitsTech::EPSP, 0) {}
};

/**
 * Implemented class: IPSP.
 */
struct Ipsp : Potential {
  Ipsp() : Potential() {}
  Ipsp(position_t pos, std::list<position_t> way, int potential_boast, int speed_boast, int duration_boast) 
    : Potential(pos, 3+potential_boast, way, IPSP_SPEED-speed_boast, UnitsTech::IPSP, IPSP_DURATION+duration_boast) {}
};

/**
 * Implemented class: Makro.
 */
struct Makro : Potential {
  Makro() : Potential() {}
  Makro(position_t pos, std::list<position_t> way, int potential_boast, int speed_boast)  
    : Potential(pos, 50+10*potential_boast, way, MACRO_SPEED-speed_boast, UnitsTech::MACRO, 0) {}
};

#endif
