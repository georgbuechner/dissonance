#ifndef SRC_PLAYER_H_
#define SRC_PLAYER_H_

#include <cstddef>
#include <deque>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <set>
#include <vector>

#include "share/audio/audio.h"
#include "share/constants/codes.h"
#include "share/constants/costs.h"
#include "share/objects/resource.h"
#include "share/objects/units.h"
#include "share/shemes/data.h"
#include "share/tools/random/random.h"
#include "share/defines.h"
#include "share/tools/utils/utils.h"

class Field;

using namespace costs;


class Player {
  public:
    /**
     * Constructor initializing all resources and gatherers with defaul values
     * and creates den with given position.
     * @param[in] username
     * @param[in] nucleus_pos position of player's nucleus.
     * @param[in] field.
     * @param[in] ran_gen (random number generator).
     * @param[in] color (random number generator).
     */
    Player(std::string username, position_t nucleus_pos, Field* field, RandomGenerator* ran_gen, int color);

    virtual ~Player() {}

    // getter:
    std::shared_ptr<Statictics> statistics() const;
    std::map<std::string, Potential> potential() const;
    int cur_range() const;
    std::map<int, Resource> resources() const;
    std::map<int, tech_of_t> technologies() const;
    std::vector<Player*> enemies() const;
    int color() const;
    int macro() const;
    virtual std::deque<AudioDataTimePoint> audio_beats() const { return {}; }
    virtual std::map<std::string, size_t> strategies() const { return {}; }

    // setter
    void set_enemies(std::vector<Player*> enemies);
    void set_lost(bool lost);

    
    // methods:
    
    // extended getter-functions

    /** 
     * Gets target of given synapse target for given potential-type.
     * @param[in] synapse_pos
     * @param[in] unit (epsp/ ipsp/ macro)
     * @return position of synapse target for given potential-type.
     */
    position_t GetSynapesTarget(position_t synapse_pos, int unit) const;

    /**
     * Gets waypoints of synapse. 
     * If unit (epsp/ ipsp/ macro) is specified, potential-target is added.
     * @param[in] synapse_pos
     * @param[in] unit (default=-1: target not added)
     * @return waypoints
     */
    std::vector<position_t> GetSynapesWaypoints(position_t synapse_pos, int unit=-1) const;

    /**
     * Gets resources as Data::Resource, to send to player.
     * @return map of resource to resource-data.
     */
    std::map<int, Data::Resource> GetResourcesInDataFormat() const;

    /**
     * Adds resource- and technology-statistics to statistics then returns statistics.
     * @return shared pointer to statistics
     */
    std::shared_ptr<Statictics> GetFinalStatistics();

     /**
     * Gets new neurons and clears new neurons.
     * @return new neurons.
     */
    std::map<position_t, int> GetNewNeurons();

    /** 
     * Gets new dead neurons and clears new-dead neurons.
     * @return new-dead neurons.
     */
    std::map<position_t, int> GetNewDeadNeurons();

    /**
     * Get neuron type at given position. 
     * @param[in] pos 
     * @return type of neuron at given position or -1 if no neuron at position.
     */
    int GetNeuronTypeAtPosition(position_t pos) const;

    /**
     * Gets map epsp-positions and number of epsps at position.
     * @return epsps-positions:number-of-epsps
     */
    std::map<position_t, int> GetEpspAtPosition() const;

    /**
     * Gets map ipsp-positions and number of ipsps at position.
     * @return ipsps-positions:number-of-ipsps
     */
    std::map<position_t, int> GetIpspAtPosition() const;

    /**
     * Gets map macro-positions and number of macros at position.
     * @return macro-positions:number-of-macros
     */
    std::map<position_t, int> GetMacroAtPosition() const;

    /**
     * Gets vector of all potential positions.
     * @return vector of all positions of potentials
     */
    std::vector<position_t> GetPotentialPositions() const;

    /**
     * Gets all neurons in cur-range to given position.
     * @param[in] pos 
     * @return list of FieldPositions (pos, unit, color) in range.
     */
    std::vector<FieldPosition> GetAllNeuronsInRange(position_t pos) const;

    /**
     * Gets vector of all positions at which player has a neuron.
     * @param[in] type (if included, returns only positions of neurons of this type)
     * @return vector of positions
     */
    std::vector<position_t> GetAllPositionsOfNeurons(int type=-1) const;

    /**
     * Gets the position of the first nucleus in unsorted dictionary of all neurons. Thus
     * returned position may change, as number of nucleus of player changes.
     * @return position of first nucleus in unsorted dictionary of all neurons. 
     */
    position_t GetOneNucleus() const;

    /**
     * Returns color at a given position: player-color for neurons and
     * potentials, resource-color for activated resource-neurons, COLOR_DEFAULT
     * otherwise.
     * @param[in] pos 
     * @return color
     */
    int GetColorForPos(position_t pos) const;

    /** 
     * Gets the position of the closet neuron of a specific type to a given position.
     * @param[in] pos 
     * @param[in] type
     * @return the position of the closet neuron of a specific type to a given position.
     */
    position_t GetPositionOfClosestNeuron(position_t pos, int unit) const;

    /**
     * Gets the string representation of the current voltage-balance of first nucleus
     * in format: "[cur_voltage] / [max_voltage]"
     * @return current voltage balance in format: "[cur_voltage] / [max_voltage]"
     */
    std::string GetNucleusLive() const;

    /**
     * Gets availible neurons or potentials to built, as list with true/ false:
     * epsp, ipsp, macro, activated-neuron, synapse, nucleus, synapse-exists
     * @return list of true/ false to indicate player-building-options.
     */
    std::vector<bool> GetBuildingOptions() const;

    /**
     * Gets availible synapse-options depending on technologies researched:
     * set-way, epsp-target, ipsp-target, macro-target, toggle-swarm-attack.
     * @return list of true/ false to indicate player-synapse-options.
     */
    std::vector<bool> GetSynapseOptions() const;
   
    /**
     * Returns missing resources for given unit.
     * @param[in] unit code for unit for which to check.
     * @param[in] boost for some costs (nucleus, technologies), costs multiply
     * if one of the type already exists. (default x1)
     * @return missing resources for this unit. Empty if all needed resources are availible.
     */
    Costs GetMissingResources(int unit, int boost=1) const;

   
    // player-actions 
    
    /**
     * Adds a newly create neuron to list of all neurons. 
     * For synapses epsp-target is given, ipsp is set to random neuron
     * (prefering activated neurons).
     * @param[in] pos position of newly added neurons.
     * @param[in] neuron (unit).
     * @param[in] epsp_target (default=DEFAULT_POS)
     * @return success/ failiure.
     */
    bool AddNeuron(position_t pos, int neuron, position_t epsp_target=DEFAULT_POS);
    
    /**
     * Adds new potential and sets it's current position and the way to it's
     * target.
     * A potential is always created with a new, random id, as (in contrast to
     * neurons) a potential is not uniquely defined by it's position.
     * @param[in] pos start position of new potential.
     * @param[in] unit should be either ESPS or IPSP.
     * @param[in] inital_speed_decrease (default=0)
     * @return success/ failiure.
     */
    bool AddPotential(position_t pos, int unit, int inital_speed_decrease=0);

    /**
     * Adds a technology if resources availible. Returns false if resources not
     * availible are other problem ocured.
     * @param[in] technology to add
     * @return boolean indicating success.
     */
    bool AddTechnology(int technology);

    /**
     * Distributes iron to resource. If distributed iron is then 2, create
     * matching resource-neuron.
     * @param[in] resource which to distribute iron to.
     * @return whether distribution was successfull.
     */
    bool DistributeIron(int resource);

    /**
     * Removes distributed iron from resource. If distributed iron is then below
     * 2, destroys matching resource-neuron.
     * @return whether removing was successfull.
     */
    bool RemoveIron(int resource);

    /**
     * Adds a waypoint to way_points of a synapse (if reset is true, resets
     * waypoints: adds given waypoint as only waypoint).
     * @param[in] pos position of synapse.
     * @param[in] way_point
     * @param[in] reset (default: false)
     * @return new number of way-points.
     */
    int AddWayPosForSynapse(position_t pos, position_t way_position, bool reset=false);

    /**
     * Switch swarm-attack on/ off.
     * @param[in] pos position of synapse.
     * @return false if no synapse found. Otherwise true if swarm activated, false otherwise.
     */
    bool SwitchSwarmAttack(position_t pos);

    /**
     * Sets target position for ipsp.
     * @param[in] position of synapse
     * @param[in] target position 
     */
    void ChangeIpspTargetForSynapse(position_t pos, position_t target_pos);

    /**
     * Sets target position for epsp.
     * @param[in] position of synapse
     * @param[in] target position 
     */
    void ChangeEpspTargetForSynapse(position_t pos, position_t target_pos);

    /**
     * Sets target position for macro.
     * @param[in] position of synapse
     * @param[in] target position 
     */
    void ChangeMacroTargetForSynapse(position_t pos, position_t target_pos);


    // main game-cycle-functions
    
    /**
     * Moves every potential forward and if it's target is reached, potential is
     * increased and the potential is removed.
     * @param[in] enemy 
     * @return potential transfered to the target.
     */
    void MovePotential();

    /**
     * Checks if loophole shall transport given potential.
     * @param[in, out] potential
     */
    void CheckLoophole(Potential& potential);

    /**
     * Handles epsp and macro action.
     * @param[in, out] potential
     */
    void HandleEpspAndMacro(Potential& potential);

    /**
     * Handles ipsp action
     * @param[in, out] potential
     */
    bool HandleIpsp(Potential& potential, std::string id);

    /**
     * Function checking whether a tower has defeted a soldier.
     */
    void HandleDef();

    /** 
     * Increases all resources by a the current boost*gain*negative-faktor.
     * gain is calculated as: `|log(current-oxygen+0.5)|`
     * @param[in] inc_iron 
     */
    void IncreaseResources(bool inc_iron);

    /**
     * Checks whether player has lost.
     * @return boolean
     */
    bool HasLost() const;


    // (resulting) actions

    /**
     * Sets block for neuron at given position.
     * @param[in] pos position of neuron
     * @param[in] block yes/ no
     */
    void SetBlockForNeuron(position_t pos, bool block);

    /**
     * Decrease potential and removes potential if potential is down to zero.
     * @param id of potential.
     * @param voltage 
     * @return whether potential was removed.
     */
    bool NeutralizePotential(std::string id, int voltage, bool erase=true);

    /**
     * Adds voltage to neuron and destroys neuron if max potential is reached.
     * @param[in] pos 
     * @param[in] potential
     */
    void AddVoltageToNeuron(position_t pos, int potential);

    /**
     * When a nucleus is destroyed, all neurons, which are not in the range of
     * another nucleus are destroyed too.
     */
    void CheckNeuronsAfterNucleusDies();
 
    /** 
     * Increase potential of neuron.
     * @param[in] potential which to add to neuron.
     * @param[in] neuron which neuron to add potential to.
     * @return whether neuron is destroyed.
     */
    bool IncreaseNeuronPotential(int potential, int neuron);

    /**
     * Updates statistics graph with final resources.
     */
    void UpdateStatisticsGraph();

    // Audio-ai
    virtual void SetUpTactics(bool) {}
    virtual void HandleIron() {}
    virtual bool DoAction() { return false; }
    virtual bool DoAction(const AudioDataTimePoint& data_at_beat) { return false; }

  protected: 
    // members 
    std::string username_;
    Field* field_;
    std::shared_ptr<Statictics> statistics_;
    RandomGenerator* ran_gen_;
    std::vector<Player*> enemies_;
    int cur_range_;
    int color_;
    bool lost_;
    int macro_; ///< type of macro (0, 1)

    // resources_
    std::map<int, Resource> resources_;
    double resource_slowdown_;  ///< negative factor when increasing resources.

    // neurons 
    std::map<position_t, std::shared_ptr<Neuron>> neurons_;
    std::map<position_t, int> new_dead_neurons_;  ///< gathers destroyed neurons
    std::map<position_t, int> new_neurons_;  ///< gathers new neurons
    std::map<int, std::vector<position_t>> neuron_positions_;  ///< stores neuron-positions

    // potentials
    std::map<std::string, Potential> potential_;

    // technologies
    std::map<int, tech_of_t> technologies_;

    
    // methods

    /**
     * Takes resources if sufficient resources exist. 
     * If `bind_resources` is set, add costs to bound resources.
     * @param[in] type 
     * @param[in] bind_resources
     * @param[in] boost (default x1)
     * @return success/ failiure
     */
    bool TakeResources(int type, bool bind_resources, int boost=1);

    /**
     * If neuron of given type is destroyed, free bound resources.
     * @param[in] type 
     */
    void FreeBoundResources(int type);

    /**
     * Increases limit for each resource by given faktor. Faktor should be
     * between 0 and 1.
     * @param[in] faktor (between -1 and 1)
     */
    void UpdateResourceLimits(float faktor);

    /**
     * Creates new neuron:
     * - adds neuron to new-neurons 
     * - adds neuron to field
     * - adds neuron to statistics 
     * - adds neuron-position to neuron-positions of type 
     * - adds neuron-positions to all neuron-positions
     */
    void CreateNeuron(int type, position_t);

    /**
     * Deletes neuron:
     * - adds neuron to new-dead-neurons 
     * - removes neuron from field 
     * - removes neuron-position from neuron-positions of type 
     * - removes neuron-position from all neuron-positions
     */
    void DeleteNeuron(int type, position_t);

    // helper

    /**
     * Gets position of a random neuron, prefering activated neurons
     * @param[in] type
     * @return Position of a random activated neuron.
     */
    position_t GetRandomNeuron();

    /**
     * Gets loophole-target if loophole at given position exists. 
     * If only-active is set, only returns target, if player has at least one macro.
     * @param[in] pos 
     * @param[in] only_active (default: false)
     * @return position of loophole target, DEFAULT_POS otherwise.
     */
    position_t GetLoopholeTargetIfExists(position_t pos, bool only_active=false) const;
};

#endif
