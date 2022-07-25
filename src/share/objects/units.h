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
#define IPSP_DURATION 24


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
    Neuron();
    Neuron(position_t pos, int lp, int type);
    Neuron(const Neuron& neuron);
    virtual ~Neuron() {}

    // getter 
    int voltage() const;
    int max_voltage() const;
    bool blocked() const;
    virtual int cur_movement() const { return 999; };
    virtual std::pair<int, int> movement() const { return DEFAULT_POS; };
    virtual int potential_slowdown() const { return -1; };
    virtual std::vector<position_t> ways_points() const { return {}; }
    virtual bool swarm() const { return false; }
    virtual unsigned int num_availible_ways() const { return 0; }
    virtual unsigned int max_stored() const { return 0; }
    virtual unsigned int stored() const { return 0; }
    virtual position_t epsp_target() const { return DEFAULT_POS; }
    virtual position_t ipsp_target() const { return DEFAULT_POS; }
    virtual position_t macro_target() const { return DEFAULT_POS; }
    virtual size_t resource() const { return 9999; }
    virtual position_t target(int unit) const { return {-1, -1}; }
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
    virtual void set_availible_ways(unsigned int num_ways) {};
    virtual void set_max_stored(unsigned int max_stored) {};
    virtual void set_target(position_t) {};

    // methods
    virtual void decrease_cooldown() {}
    virtual void reset_movement() {}

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
    Synapse(const Neuron& neuron);

    // getter: 
    std::vector<position_t> ways_points() const;
    bool swarm() const;
    unsigned int num_availible_ways() const;
    unsigned int max_stored() const;
    unsigned int stored() const;
    position_t epsp_target() const { return epsp_target_; }
    position_t ipsp_target() const { return ipsp_target_; }
    position_t macro_target() const { return macro_target_; }

    position_t target(int unit)  const { 
      if (unit == IPSP) return ipsp_target_;
      if (unit == EPSP) return epsp_target_;
      if (unit == MACRO) return macro_target_;
      return {-1, -1};
    }
   
    // setter: 
    void set_way_points(std::vector<position_t> way_points);
    void set_swarm(bool swarm);
    void set_epsp_target_pos(position_t pos);
    void set_ipsp_target_pos(position_t pos);
    void set_macro_target_pos(position_t pos);
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
    position_t macro_target_;

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
    ActivatedNeuron(const Neuron& neuron);

    // getter 
    int cur_movement() const;
    std::pair<int, int> movement() const;
    int potential_slowdown() const;
    
    void decrease_cooldown();
    void reset_movement();

  private:
    std::pair<int, int> movement_;  ///< lower number means higher speed.
    int potential_slowdown_;
};

/**
 *
 */
struct ResourceNeuron : Neuron {
  public: 
    ResourceNeuron();
    ResourceNeuron(position_t, size_t resource);
    ResourceNeuron(const Neuron& neuron);

    // getter 
    size_t resource() const;

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
  Nucleus(const Neuron& neuron) : Neuron(neuron) {
    spdlog::get(LOGGER)->debug("Added Nucleus. type: {}", type_);
  }
};

struct Loophole : Neuron {
  public: 
    Loophole() : Neuron(), target_({-1, -1}), created_at_(std::chrono::steady_clock::now()) {}
    Loophole(position_t pos, position_t target) : Neuron(pos, 9, UnitsTech::LOOPHOLE), target_(target) {}

    // getteconst r
    position_t target() const { return target_; }
    std::chrono::time_point<std::chrono::steady_clock> created_at() const { return created_at_; }

    // setter
    void set_target(position_t pos) { target_ = pos; }

  private:
    position_t target_;
    const std::chrono::time_point<std::chrono::steady_clock> created_at_;
};

/**
 * Abstrackt class for all potentials.
 * Attributes:
 * - pos (derived from Unit)
 * - cur_movement (int cooldown, int speed)
 */
struct Potential : Unit {
  int potential_;
  std::pair<int, int> movement_;  ///< lower number means higher speed.
  int duration_; ///< only ipsp
  std::list<position_t> way_;
  bool target_blocked_;

  Potential() : Unit(), movement_({999, 999}) {}
  Potential(position_t pos, int attack, std::list<position_t> way, int speed, int type, int duration) 
    : Unit(pos, type), potential_(attack), movement_({speed, speed}), duration_(duration), 
    way_(way), target_blocked_(false) {}
};

/**
 * Implemented class: EPSP
 * Attributes:
 * - pos (derived from Unit)
 * - attack (derived from Potential)
 * - speed (derived from Potential)
 * - way (derived from Potential)
 */
struct Epsp : Potential {
  Epsp() : Potential() {}
  Epsp(position_t pos, std::list<position_t> way, int potential_boast, int speed_boast) 
    : Potential(pos, 2+potential_boast, way, EPSP_SPEED-speed_boast, UnitsTech::EPSP, 0) {}
};

/**
 * Implemented class: IPSP.
 * Attributes:
 * - pos (derived from Unit)
 * - attack (derived from Potential)
 * - speed (derived from Potential)
 * - way (derived from Potential)
 */
struct Ipsp : Potential {
  Ipsp() : Potential() {}
  Ipsp(position_t pos, std::list<position_t> way, int potential_boast, int speed_boast, int duration_boast) 
    : Potential(pos, 3+potential_boast, way, IPSP_SPEED-speed_boast, UnitsTech::IPSP, IPSP_DURATION+duration_boast) {}
};

struct Makro : Potential {
  Makro() : Potential() {}
  Makro(position_t pos, std::list<position_t> way, int potential_boast, int speed_boast)  
    : Potential(pos, 50+10*potential_boast, way, MACRO_SPEED-speed_boast, UnitsTech::MACRO, 0) {}
};

#endif
