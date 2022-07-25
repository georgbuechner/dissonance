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
    struct AiOption {
      int _type;
      position_t _pos;
      position_t _pos_2;
      float _num;

      std::string string() const { return std::to_string(_type) + ": pos1:" + utils::PositionToString(_pos) 
        + ", pos2: " + utils::PositionToString(_pos_2) + ", num: " + utils::Dtos(_num); }

      std::string hash() const { return std::to_string(_type) + ";" + utils::PositionToString(_pos) + ";"
        + utils::PositionToString(_pos_2) + ";" + std::to_string(_num); }
      AiOption(std::string action) {
        auto parts = utils::Split(action, ";");
        _type = std::stoi(parts[0]);
        _pos = utils::PositionFromString(parts[1]);
        _pos_2 = utils::PositionFromString(parts[2]);
        _num = std::atof(parts[3].c_str());
      }
      AiOption(int type, position_t pos, position_t pos2, float num) {
        _type = type;
        _pos = pos;
        _pos_2 = pos2;
        _num = num;
      }
    };

    struct McNode {
      int _count;
      float cur_weight_;
      std::map<std::string, McNode*> _nodes;

      void update(float weight) {
        _count++;
        cur_weight_ += weight;
      }
    };

    /**
     * Constructor initializing all resources and gatherers with defaul values
     * and creates den with given position.
     * @param[in] nucleus_pos position of player's nucleus.
     * @param[in] field.
     * @param[in] ran_gen (random number generator).
     * @param[in] color (random number generator).
     */
    Player(position_t nucleus_pos, Field* field, RandomGenerator* ran_gen, int color);
    Player(const Player& p, Field* field);

    virtual ~Player() {}

    // getter:
    std::shared_ptr<Statictics> statistics();
    std::map<std::string, Potential> potential();
    int cur_range();
    std::map<int, Resource> resources();
    std::map<int, tech_of_t> technologies();

    std::map<int, Data::Resource> t_resources();
    std::map<position_t, int> new_dead_neurons();
    std::map<position_t, int> new_neurons();
    std::vector<Player*> enemies();
    int color();
    virtual std::deque<AudioDataTimePoint> data_per_beat() const { return {}; }

    position_t GetSynapesTarget(position_t synapse_pos, int unit);
    std::vector<position_t> GetSynapesWayPoints(position_t synapse_pos, int unit=-1);
    int macro() { return macro_; }

    // setter
    void set_enemies(std::vector<Player*> enemies);
    void set_lost(bool lost);

    // methods:
    
    std::shared_ptr<Statictics> GetFinalStatistics(std::string username);
    std::map<position_t, int> GetEpspAtPosition();
    std::map<position_t, int> GetIpspAtPosition();
    std::map<position_t, int> GetMacroAtPosition();
    std::vector<position_t> GetPotentialPositions();
    std::vector<FieldPosition> GetAllNeuronsInRange(position_t pos);
    position_t GetLoopholeTargetIfExists(position_t pos, bool only_active=false);
    int GetColorForPos(position_t pos);

    /** 
     * Gets the position of the closet neuron of a specific type to a given position.
     * @param[in] pos 
     * @param[in] type
     * @return the position of the closet neuron of a specific type to a given position.
     */
    position_t GetPositionOfClosestNeuron(position_t pos, int unit);

    /**
     * Gets the string representation of the current voltage balance in format: "[cur_voltage] / [max_voltage]"
     * @return current voltage balance in format: "[cur_voltage] / [max_voltage]"
     */
    std::string GetNucleusLive();
    int GetNucleusVoltage();
    
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
    int GetNeuronTypeAtPosition(position_t pos);

    /**
     * Checks whether neuron at given position is currently blocked.
     * @param[in] pos
     * @returns whether a neuron at a given position is blocked.
     */
    bool IsNeuronBlocked(position_t pos);

    /**
     * Gets vector of all position at which player has a nucleus.
     * @param[in] type (if included, returns only positions of neurons of this type)
     * @return vector of positions
     */
    std::vector<position_t> GetAllPositionsOfNeurons(int type=-1);

    /**
     * Gets position of a random activated neuron.
     * @param[in] type
     * @return Position of a random activated neuron.
     */
    position_t GetRandomNeuron(std::vector<int> type={UnitsTech::ACTIVATEDNEURON});

    /**
     * Gets the position of the first nucleus in unsorted dictionary of all neurons. Thus
     * returned position may change, as number of nucleus of player changes.
     * @return position of first nucleus in unsorted dictionary of all neurons. 
     */
    position_t GetOneNucleus();

    /**
     * Resets way-points for synapse at given position to contain only given way-point.
     * @param[in] pos position of synapse.
     * @param[in] way_point
     * @return new number of way-points.
     */
    int ResetWayForSynapse(position_t pos, position_t way_point);

    /**
     * Adds a way point to way_points of a synapse.
     * @param[in] pos position of synapse.
     * @param[in] way_point
     * @return new number of way-points.
     */
    int AddWayPosForSynapse(position_t pos, position_t way_position);

    /**
     * Switch swarm-attack on/ off.
     * @param[in] pos position of synapse.
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

    std::vector<bool> GetBuildingOptions();
    std::vector<bool> GetSynapseOptions();
   
    /** 
     * Increases all resources by a the current boast*gain*negative-faktor.
     * gain is calculated as: `|log(current-oxygen+0.5)|`
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

    bool RemoveIron(int resource);

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
    bool AddNeuron(position_t pos, int neuron, position_t epsp_target={-1, -1}, position_t ipsp_target={-1, -1});

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
    bool AddPotential(position_t pos, int unit, int inital_speed_decrease=0);

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
    void MovePotential();
    void CheckLoophole(Potential& potential);
    void HandleEpspAndMacro(Potential& potential);
    bool HandleIpsp(Potential& potential, std::string id);

    void SetBlockForNeuron(position_t pos, bool block);

    /**
     * Function checking whether a tower has defeted a soldier.
     */
    void HandleDef();

    /**
     * Decrease potential and removes potential if potential is down to zero.
     * @param id of potential.
     * @return whether potential was removed.
     */
    bool NeutralizePotential(std::string id, int potential, bool erase=true);

    /**
     * Adds potential to neuron and destroies neuron if max potential is reached.
     * @param[in] pos 
     * @param[in] potential
     */
    void AddPotentialToNeuron(position_t pos, int potential);

    /**
     * When a nucleus is destroyed, all neurons, which are not in the range of
     * another nucleus are destroyed too.
     */
    void CheckNeuronsAfterNucleusDies();
 
    /**
     * Returns id of potential iof unit at given position is potential.
     * @param[in] pos position to check for.
     */
    std::string GetPotentialIdIfPotential(position_t pos, int unit=-1);

    /** 
     * Increase potential of neuron.
     * @param[in] potential which to add to neuron.
     * @param[in] neuron which neuron to add potential to.
     * @return whether neuron is destroyed.
     */
    bool IncreaseNeuronPotential(int potential, int neuron);

    std::string GetCurrentResources();

    // Audio-ai
    virtual void SetUpTactics(bool) {}
    virtual void HandleIron(const AudioDataTimePoint&) {}
    virtual bool DoAction() { return false; }
    virtual bool DoAction(const AudioDataTimePoint& data_at_beat) { return false; }

  protected: 
    Field* field_;
    std::shared_ptr<Statictics> statistics_;
    RandomGenerator* ran_gen_;
    std::vector<Player*> enemies_;
    int cur_range_;
    int color_;
    bool lost_;
    int macro_;

    std::map<int, Resource> resources_;
    double resource_slowdown_;


    std::map<position_t, std::shared_ptr<Neuron>> neurons_;
    std::map<position_t, int> new_dead_neurons_;
    std::map<position_t, int> new_neurons_;
    std::map<int, std::vector<position_t>> neuron_positions_;

    std::map<std::string, Potential> potential_;

    std::map<int, tech_of_t> technologies_;

    // methods
    bool TakeResources(int type, bool bind_resources, int boast=1);
    void FreeBoundResources(int type);

    /**
     * Increases limit for each resource by given faktor. Faktor should be
     * between 0 and 1.
     * @param[in] faktor (between -1 and 1)
     */
    void UpdateResourceLimits(float faktor);

    void CreateNeuron(int type, position_t);
    void DeleteNeuron(int type, position_t);
};

#endif
