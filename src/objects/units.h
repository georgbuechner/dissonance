#ifndef SRC_SOLDIER_H_
#define SRC_SOLDIER_H_

#include <chrono>
#include <iostream>
#include <list>
#include <stdexcept>
#include <vector>
#include <spdlog/spdlog.h>

#include "constants/codes.h"

#define LOGGER "logger"

typedef std::pair<int, int> Position;

/**
 * Abstrackt class for all neurons or potentials.
 * Attributes:
 * - pos
 */
struct Unit {
  Position pos_;
  int type_;

  Unit() : pos_({}) {}
  Unit(Position pos, int type) : pos_(pos), type_(type) {}
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
    virtual std::vector<Position> ways_points() {return {}; }
    virtual bool swarm() { return false; }
    virtual unsigned int num_availible_ways() { return 0; }
    virtual unsigned int max_stored() { return 0; }

    // setter
    void set_blocked(bool blocked);
    virtual void set_last_action(std::chrono::time_point<std::chrono::steady_clock> time) {};
    virtual void set_way_points(std::vector<Position> pos) {};
    virtual void set_swarm(bool swarm) {};
    virtual void set_epsp_target_pos(Position pos) {};
    virtual void set_ipsp_target_pos(Position pos) {};
    virtual void set_availible_ways(unsigned int num_ways) {};
    virtual void set_max_stored(unsigned int max_stored) {};

    // methods

    /**
     * Increases voltage of neuron and returns whether neuron is destroyed.
     */
    bool IncreaseVoltage(int potential);

    virtual std::vector<Position> GetWayPoints(int unit) const { return {}; }
    virtual unsigned int AddEpsp() { return 0; }

    Neuron();
    Neuron(Position pos, int lp, int type);
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
    Synapse(Position pos, int max_stored, int num_availible_ways, Position epsp_target, Position ipsp_target);

    // getter: 
    std::vector<Position> ways_points();
    bool swarm();
    unsigned int num_availible_ways();
    unsigned int max_stored();
   
    // setter: 
    void set_way_points(std::vector<Position> way_points);
    void set_swarm(bool swarm);
    void set_epsp_target_pos(Position pos);
    void set_ipsp_target_pos(Position pos);
    void set_availible_ways(unsigned int num_availible_way_points);
    void set_max_stored(unsigned int max_stored);

    // methods: 
    std::vector<Position> GetWayPoints(int unit) const;
    unsigned int AddEpsp();

  private:
    bool swarm_;
    int max_stored_;
    int stored_;
    
    Position epsp_target_;
    Position ipsp_target_;

    unsigned int num_availible_way_points_;
    std::vector<Position> way_points_;
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
    ActivatedNeuron(Position pos, int slowdown_boast, int speed_boast);

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
 * Implemented class: Synapse.
 * The Nucleus is the main point from which your system is developing.
 * Attributes:
 * - pos (derived from Unit)
 * - lp (derived from Neuron)
 */
struct Nucleus : Neuron {
  Nucleus() : Neuron() {}
  Nucleus(Position pos) : Neuron(pos, 9, UnitsTech::NUCLEUS) {}
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
  std::list<Position> way_;

  Potential() : Unit(), speed_(999), last_action_(std::chrono::steady_clock::now()) {}
  Potential(Position pos, int attack, std::list<Position> way, int speed, int type, int duration) 
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
  Epsp(Position pos, std::list<Position> way, int potential_boast, int speed_boast) 
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
  Ipsp(Position pos, std::list<Position> way, int potential_boast, int speed_boast, int duration_boast) 
    : Potential(pos, 3+potential_boast, way, 420-speed_boast, UnitsTech::IPSP, 4+duration_boast) {}
};

#endif
