#ifndef SRC_PLAYER_H_
#define SRC_PLAYER_H_

#include <cstddef>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <set>
#include <shared_mutex>
#include <vector>

#include "constants/codes.h"
#include "constants/costs.h"
#include "objects/data_structs.h"
#include "objects/units.h"

class Field;

using namespace costs;

#define COLOR_AVAILIBLE 1
#define COLOR_ERROR 2 
#define COLOR_DEFAULT 3
#define COLOR_MSG 4
#define COLOR_SUCCESS 5
#define COLOR_MARKED 6

typedef std::map<size_t, std::pair<std::string, int>> choice_mapping_t;
typedef std::vector<std::vector<std::string>> Paragraphs;
typedef std::pair<int, int> Position;
typedef std::pair<double, bool> Resource;
typedef std::pair<size_t, size_t> TechXOf;

class Player {
  public:

    /**
     * Constructor initializing all resources and gatherers with defaul values
     * and creates den with given position.
     * @param[in] nucleus_pos position of player's nucleus.
     * @param[in] silver initial silver value.
     */
    Player(Position nucleus_pos, Field* field, int iron);

    // getter:
    std::map<std::string, Potential> potential();
    Position nucleus_pos();
    int cur_range();
    int iron();
    int resource_curve();
    int bound_oxygen();
    int max_oxygen();
    int oxygen_boast();
    std::map<int, Resource> resources();
    std::map<int, TechXOf> technologies();

    // setter
    void set_resource_curve(int resource_curve);
    void set_iron(int iron);

    // methods:
    Position GetPositionOfClosestNeuron(Position pos, int unit) const;
    std::string GetNucleusLive();
    
    /**
     * Checks whether player has lost.
     * @return boolean
     */
    bool HasLost();

    /**
     * Get neuron type at given position. 
     * @param[in] pos 
     * @return returns type of neuron at given position or -1 if no neuron at position.
     */
    int GetNeuronTypeAtPosition(Position pos);

    /**
     * Checks whether neuron at given position is currently blocked.
     * @param[in] pos
     * @returns whether a neuron at a given position is blocked.
     */
    bool IsNeuronBlocked(Position pos);

    /**
     * Gets vector of all position at which player has a nucleus.
     * @param[in] type (if included, returns only positions of neurons of this type)
     * @return vector of positions
     */
    std::vector<Position> GetAllPositionsOfNeurons(int type=-1);

    /**
     * Gets position of a random activated neuron.
     * @return Position of a random activated neuron.
     */
    Position GetRandomActivatedNeuron();

    /**
     * Resets way-points for synapse at given position to contain only given way-point.
     * @param[in] pos position of synapse.
     * @param[in] way_point
     */
    void ResetWayForSynapse(Position pos, Position way_point);

    /**
     * Adds a way point to way_points of a synapse.
     * @param[in] pos position of synapse.
     * @param[in] way_point
     */
    void AddWayPosForSynapse(Position pos, Position way_position);

    /**
     * Switch swarm-attack on/ off.
     * @param[in] pos position of synapse.
     */
    void SwitchSwarmAttack(Position pos);

    /**
     * Sets target position for ipsp.
     * @param[in] position of synapse
     * @param[in] target position 
     */
    void ChangeIpspTargetForSynapse(Position pos, Position target_pos);
    /**
     * Sets target position for epsp.
     * @param[in] position of synapse
     * @param[in] target position 
     */
    void ChangeEpspTargetForSynapse(Position pos, Position target_pos);

    /**
     * Gets availible option (depending of current synapse-state and
     * technologies) for synapse.
     * @param[in] pos position of synapse
     * @return options.
     */
    choice_mapping_t GetOptionsForSynapes(Position pos);
   
    /**
     * Show current status (resources, gatherers, den-lp ...)
     */
    std::vector<std::string> GetCurrentStatusLine();

    /** 
     * Increase all resources by set amount.
     * @param[in] inc_iron 
     */
    void IncreaseResources(bool inc_iron);

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
    Costs GetMissingResources(int unit, int boast=1);

    /**
     * Adds a newly create neuron to list of all neurons.
     * @param[in] pos position of newly added neurons.
     * @param[in] neuron (unit).
     * @return success/ failiure.
     */
    bool AddNeuron(Position pos, int neuron, Position epsp_target={-1, -1}, Position ipsp_target={-1, -1});

    /**
     * Adds new potential and sets it's current position and the way to it's
     * target.
     * A potential is always created with a new, random id, as (in contrast to
     * neurons) a potential is not uniquely defined by it's position.
     * @param[in] pos start position of new potential.
     * @param[in] way to the enemies neuron.
     * @param[in] unit should be either ESPS or IPSP.
     * @return success/ failiure.
     */
    bool AddPotential(Position pos, int unit);

    /**
     * Adds a technology if resources availible. Returns false if resources not
     * availible are other problem ocured.
     * @param[in] technology to add
     * @return boolean indicating success.
     */
    bool AddTechnology(int technology);

    /**
     * Moves every potential forward and if it's target is reached, potential is
     * increased and the potential is removed.
     * @param[in] enemy 
     * @return potential transfered to the target.
     */
    void MovePotential(Player* enemy);

    void SetBlockForNeuron(Position pos, bool block);

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
    void NeutralizePotential(std::string id, int potential);

    /**
     * Adds potential to neuron and destroies neuron if max potential is reached.
     * @param[in] pos 
     * @param[in] potential
     */
    void AddPotentialToNeuron(Position pos, int potential);

    /**
     * When a nucleus is destroyed, all neurons, which are not in the range of
     * another nucleus are destroyed too.
     */
    void CheckNeuronsAfterNucleusDies();
 
    /**
     * Returns id of potential iof unit at given position is potential.
     * @param[in] pos position to check for.
     */
    std::string GetPotentialIdIfPotential(Position pos, int unit=-1);

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


  protected: 
    Field* field_;
    int cur_range_;

    std::map<int, Resource> resources_;
    int resource_curve_;
    int oxygen_boast_;
    int max_oxygen_;
    int max_resources_;
    double total_oxygen_;
    double bound_oxygen_;
    std::shared_mutex mutex_resources_;

    std::shared_mutex mutex_nucleus_;
    Nucleus nucleus_;
    std::shared_mutex mutex_all_neurons_;
    std::map<Position, std::shared_ptr<Neuron>> neurons_;

    std::shared_mutex mutex_potentials_;
    std::map<std::string, Potential> potential_;

    std::shared_mutex mutex_technologies_;
    std::map<int, TechXOf> technologies_;

    // methods
    bool TakeResources(int type, int boast=1);
    double Faktor(int limit, double cur);

    /**
     * Gets a neuron at given position. If unit is specified, only returns, if
     * the neuron at the given position is this unit.
     * Throws invalid_argument exception if no neuron at position or wrong unit.
     * @param[in] pos 
     * @param[in] unit
     * @return Neuron at given position.
     */
    std::shared_ptr<Neuron> GetNeuron(Position pos, int unit=-1);
};

#endif
