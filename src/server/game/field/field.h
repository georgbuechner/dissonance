#ifndef SRC_FIELD_H_
#define SRC_FIELD_H_

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <shared_mutex>
#include <utility>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "share/objects/units.h"
#include "server/game/player/player.h"
#include "share/tools/graph.h"
#include "share/tools/random/random.h"
#include "share/defines.h"

class Field {
  public:
    /** 
     * Constructor initializing field with availible lines and columns
     * @param[in] lines availible lines.
     * @param[in] cols availible cols
     */
    Field(int lines, int cols, RandomGenerator* ran_gen);
    Field(const Field& field);

    // getter 
    int lines();
    int cols();
    std::shared_ptr<Graph> graph();
    const std::map<position_t, std::map<int, position_t>>& resource_neurons();
    const std::map<position_t, std::vector<std::pair<std::string, Player*>>>& epsps();

    // methods:
    
    /**
     * Gets all positions in graph as vector.
     * @return vector of all positions in graph.
     */
    std::vector<position_t> GraphPositions();
    
    /**
     * Adds random natural barriers based on pitch
     * @param[in] reduced_pitches (ruffly matching number of positions in field)
     * @param[in] looseness can be increased to create less 'hills' assure that map is playable.
     */
    void AddHills(std::vector<double> reduced_pitches, int looseness);

    /**
     * Adds nucleus and resources for each player.
     * @param[in] num_players 
     * @return nucleus-position for each player.
     */
    std::vector<position_t> AddNucleus(int num_players);

    /** 
     * Adds resources.
     * @param[in] start_pos position near which to create resources.
     */
    void AddResources(position_t start_pos);

    /**
     * Builds graph to calculate ways.
     * Function should be called after field is initialized. Function checks all
     * fields reachable from player-den (this could be the den of any player).
     */
    void BuildGraph();

    /**
     * Adds new neuron to field at given position (not for resource-neurons) and
     * adds to field-neurons together with player.
     * @param[in] pos position of new neuron
     * @param[in] neuron shared pointer to neuron
     * @param[in] p player neuron belongs to
     */
    void AddNewNeuron(position_t pos, std::shared_ptr<Neuron> neuron, Player* p);

    /**
     * Removes neuron from map of neurons, does not removes from field, as
     * position if indefinetly taken.
     * @param[in] pos
     */
    void RemoveNeuron(position_t pos);

    /**
     * Gets neuron-type and matching player at given position.
     * @param[in] pos
     * @return pair of neuron-type and player
     */
    std::pair<int, Player*> GetNeuronTypeAndPlayerAtPosition(position_t pos);

    /**
     * Gets way to a soldiers target.
     * @param[in] start_pos starting position.
     * @param[in] target_pos target position.
     * @return list of positions.
     */ 
    std::list<position_t> GetWay(position_t start_pos, const std::vector<position_t>& way_points);

    /** 
     * Finds the next free position near a given position with min and max deviation.
     * @param pos
     * @param min distance 
     * @param max distance
     * @return free position in given range.
     */
    position_t FindFree(position_t pos, int min, int max);

    /**
     * Gets symbol at requested position.
     * @param[in] position to check.
     * @return symbol at requested position.
     */
    std::string GetSymbolAtPos(position_t pos) const;

    /**
     * Gets all positions with a certain distance (min, max) to a given start-point. 
     * If specified, returns only free positions.
     * @param[in] start position
     * @param[in] max_dist 
     * @param[in] min_dist
     * @param[in] free (default: false=don't check if positions are free)
     * @return array of position in specified range.
     */
    std::vector<position_t> GetAllInRange(position_t start, double max_dist, double min_dist, bool free=false);

    /**
     * Gets the positions of the center of each section.
     * @return positions of the center of each section.
     */
    std::vector<position_t> GetAllCenterPositionsOfSections();

    /**
     * Gets all positions of a given section.
     * @param[in] section
     * @param[in] in_graph (if specified retruns only positions within graph)
     * @return all positions of a given section.
     */
    std::vector<position_t> GetAllPositionsOfSection(int section, bool in_graph = false);

    /**
     * Converts current field to transfer-object.
     * @param[in] players
     * @return two-dimensional array with Symbol (string, color) as value.
     */
    std::vector<std::vector<Data::Symbol>> Export(std::vector<Player*> players);

    /**
     * Calls `Export(std::vector<Player*>)` after converting map to list.
     * @param[in] players
     * @return two-dimensional array with Symbol (string, color) as value.
     */
    std::vector<std::vector<Data::Symbol>> Export(std::map<std::string, Player*> players);

    /**
     * Gathers and stores all epsps from all players after clearing current
     * gathered epsps.
     * @param[in] players
     */
    void GatherEpspsFromPlayers(std::map<std::string, Player*> players);

  private: 
    // members
    int lines_;
    int cols_;
    RandomGenerator* ran_gen_;
    std::shared_ptr<Graph> graph_;
    std::vector<std::vector<std::string>> field_;
    std::map<position_t, std::map<int, position_t>> resource_neurons_;  ///< stored to access after creation
    std::map<position_t, std::pair<std::shared_ptr<Neuron>, Player*>> neurons_;  ///< stored here for fast-access
    std::map<position_t, std::vector<std::pair<std::string, Player*>>> epsps_;  ///< stored here for fast-access

    // functions
    
    /**
     * Checks whether given position is inside of field.
     * @param pos
     * @return boolen indicating, whether given position is inside of field.
     */
    bool InField(position_t pos);

    /**
     * Checks if a nucleus is within given range of position.
     * @param[in] pos 
     * @param[in] range 
     * @return whether a nucleus is within range of position.
     */
    bool NucleusInRange(position_t pos, int range);
};

#endif
