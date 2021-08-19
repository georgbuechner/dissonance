#ifndef SRC_PLAYER_H_
#define SRC_PLAYER_H_

#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <set>
#include <shared_mutex>
#include <vector>

#include "codes.h"
#include "units.h"

typedef std::vector<std::vector<std::string>> Paragraphs;
typedef std::pair<int, int> Position;
typedef std::pair<double, bool> Resource;
typedef const std::map<int, double> Costs;

class Player {
  public:

    /**
     * Constructor initializing all resources and gatherers with defaul values
     * and creates den with given position.
     * @param[in] nucleus_pos position of player's nucleus.
     * @param[in] silver initial silver value.
     */
    Player(Position nucleus_pos, int iron);

    // getter:
    std::map<std::string, Epsp> epsps();
    std::map<std::string, Ipsp> ipsps();
    std::map<Position, Synapse> synapses();
    std::map<Position, ActivatedNeuron> activated_neurons();
    std::set<Position> neurons();
    Position nucleus_pos();
    int nucleus_potential();
    int cur_range();
    int iron();
    int resource_curve();
    std::map<int, Resource> resources();

    // setter
    void set_resource_curve(int resource_curve);

    // methods:
   
    /**
     * Show current status (resources, gatherers, den-lp ...)
     */
    std::string GetCurrentStatusLineA();
    std::string GetCurrentStatusLineB();
    std::string GetCurrentStatusLineC();

    /** 
     * Increase all resources by set amount.
     */
    void IncreaseResources();

    /**
     * Distributes iron to either boast oxygen production or activate neu
     * resource.
     * @param resource which to distribute iron to.
     * @return whether distribution was successfull.
     */
    bool DistributeIron(int resource);

    /**
     * Returns missing resources for given unit.
     * @param unit code for unit for which to check.
     * @return missing resources for this unit. Empty if all needed resources
     * are availible.
     */
    Costs CheckResources(int unit);

    /**
     * Adds a newly create neuron to list of all neurons.
     * @param[in] pos position of newly added neurons.
     * @param[in] neuron (unit).
     */
    void AddNeuron(Position pos, int neuron);

    /**
     * Adds new potential and sets it's current position and the way to it's
     * target.
     * A potential is always created with a new, random id, as (in contrast to
     * neurons) a potential is not uniquely defined by it's position.
     * @param[in] pos start position of new potential.
     * @param[in] way to the enemies neuron.
     * @param[in] unit should be either ESPS or IPSP.
     */
    void AddPotential(Position pos, std::list<Position> way, int unit);

    /**
     * Moves every potential forward and if it's target is reached, potential is
     * increased and the potential is removed.
     * @param[in] target_pos position of targeted enemy neuron.
     * @return potential transfered to the target.
     */
    int MovePotential(Position target_pos, Player* enemy);

    void SetBlockForNeuron(Position pos, int unit, bool block);

    /**
     * Function checking whether a tower has defeted a soldier.
     * @param player 
     * @param ki_
     */
    void HandleDef(Player* enemy);

    /**
     * Decrease potential and removes potential if potential is down to zero.
     * @param id of potential.
     */
    void NeutalizePotential(std::string id);
 
    /**
     * Checks if a potential on the map belongs to this player.
     * @param[in] pos position to check for.
     */
    bool IsSoldier(Position pos, int unit=-1);

    /** 
     * Checks if a resource is activated.
     * @param[in] resource which to check
     * @return whether resource is activated.
     */
    bool IsActivatedResource(int resource);

    /** 
     * Increase potential of neuron.
     * @param[in] potential which to add to neuron.
     * @param[in] neuron which neuron to add potential to.
     * @return whether neuron is destroyed.
     */
    bool IncreaseNeuronPotential(int potential, int neuron);

  private: 
    int cur_range_;
    int resource_curve_;
    int oxygen_boast_;
    double total_oxygen_;
    double bound_oxygen_;
    std::map<int, Resource> resources_;
    std::shared_mutex mutex_resources_;

    std::chrono::time_point<std::chrono::steady_clock> last_iron_; 

    std::shared_mutex mutex_nucleus_;
    Nucleus nucleus_;

    std::shared_mutex mutex_potentials_;
    std::map<std::string, Epsp> epsps_;
    std::map<std::string, Ipsp> ipsps_;

    std::shared_mutex mutex_all_neurons_;
    std::set<Position> all_neurons_;  ///< simple set, to check whether position belongs to player.
    std::map<Position, Synapse> synapses_;
    std::map<Position, ActivatedNeuron> activated_neurons_;

    // methods
    void TakeResources(Costs costs);
    double Faktor(int limit, double cur);
};

#endif
