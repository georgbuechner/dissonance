#ifndef SRC_FIELD_H_
#define SRC_FIELD_H_

#include <map>
#include <mutex>
#include <string>
#include <shared_mutex>
#include <utility>
#include <vector>
#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "nlohmann/json_fwd.hpp"
#include "share/objects/transfer.h"
#include "share/objects/units.h"
#include "server/game/player/player.h"
#include "share/tools/graph.h"
#include "share/tools/random/random.h"
#include "share/defines.h"

class Field {
  public:
    /** Constructor initializing field with availible lines and columns
     * @param[in] lines availible lines.
     * @param[in] cols availible cols
     */
    Field(int lines, int cols, RandomGenerator* ran_gen);

    // methods:
    
    /**
     * Adds random natural barriers.
     */
    void AddHills(RandomGenerator*, RandomGenerator*, unsigned short denceness=1);

    /** 
     * Adds resources ([G]old, [S]ilver, [B]ronze) near given position.
     * @param start_pos position near which to create resources.
     */
    std::map<int, position_t> AddResources(position_t start_pos);

    /**
     * Adds position for player nucleus.
     * @param[in] section
     * @return position of player nucleus.
     */
    position_t AddNucleus(int section);

    /**
     * Builds graph to calculate ways.
     * Function should be called after field is initialized. Function checks all
     * fields reachable from player-den (this could be the den of any player).
     * @param[in] player_nucleus (list to check if each nucleus is reachable).
     */
    void BuildGraph(std::vector<position_t> player_nucleus);

    /**
     * Finds a free position for a defence tower and adds tower to player/ ki
     * units.
     * @param pos position of new defence tower.
     * @param unit integer to identify unit.
     */
    void AddNewUnitToPos(position_t pos, int unit);

    /**
     * Gets way to a soldiers target.
     * @param start_pos starting position.
     * @param target_pos target position.
     * @return list of positions.
     */ 
    std::list<position_t> GetWayForSoldier(position_t start_pos, std::vector<position_t> way_points);

    /** 
     * Finds the next free position near a given position with min and max
     * deviation.
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
    std::vector<position_t> GetAllInRange(position_t start, double max_dist, 
        double min_dist, bool free=false);

    /**
     * Gets all positions of a given section.
     * @param[in] section]
     * @return all positions of a given section.
     */
    std::vector<position_t> GetAllPositionsOfSection(unsigned short section);

    /**
     * Converts current field to transfer-object.
     * @param[in] players
     * @return two-dimensional array with Symbol (string, color) as value.
     */
    std::vector<std::vector<Transfer::Symbol>> Export(std::vector<Player*> players);

  private: 
    // members
    int lines_;
    int cols_;
    RandomGenerator* ran_gen_;
    Graph graph_;
    std::vector<std::vector<std::string>> field_;
    std::shared_mutex mutex_field_;

    // functions

    /**
     * Checks whether given position is inside of field.
     * @param pos
     * @return boolen indicating, whether given position is inside of field.
     */
    bool InField(position_t pos);
};


#endif
