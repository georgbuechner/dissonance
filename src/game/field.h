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
#include "objects/transfer.h"
#include "player/player.h"
#include "utils/graph.h"
#include "objects/units.h"
#include "random/random.h"

class Field {
  public:
    /** Constructor initializing field with availible lines and columns
     * @param[in] lines availible lines.
     * @param[in] cols availible cols
     */
    Field(int lines, int cols, RandomGenerator* ran_gen);

    // getter:
    int lines();
    int cols();

    // methods:
    
    void AddBlink(position_t pos);

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
     * Check whether a given position is in given range (euklidean distance), or in graph.
     * @param[in] pos (position to check)
     * @param[in] range max euklidean distance, or ViewRange::GRAPH -> check graph.
     * @param[in] start start position (if checking euklidean distance)
     * @return wether given position is in range.
     */
    bool InRange(position_t pos, int range, position_t start = {-1, -1});

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
     * Gets the positions of the center of each section.
     * @return positions of the center of each section.
     */
    std::vector<position_t> GetAllCenterPositionsOfSections();

    /**
     * Gets all positions of a given section.
     * @param[in] section]
     * @return all positions of a given section.
     */
    std::vector<position_t> GetAllPositionsOfSection(unsigned short section);

    std::vector<std::vector<Transfer::Symbol>> ToJson(std::vector<Player*> players);

  private: 
    // members
    int left_border_;
    int lines_;
    int cols_;
    RandomGenerator* ran_gen_;
    Graph graph_;
    std::vector<std::vector<std::string>> field_;
    std::shared_mutex mutex_field_;

    std::vector<position_t> blinks_;

    // functions

    /**
     * Places moving units (soldiers) on the field, by taking units from given
     * player. 
     * Usually this function is called for each player just before printing the
     * field. This function only modifies a temporary copy of the field.
     * @param[in] player player of which to add soldiers.
     * @param[out] field reference to copy of field.
     */
    void UpdateField(Player* player, std::vector<std::vector<std::string>>& field);

    bool CheckCollidingPotentials(position_t pos, std::vector<Player*> players);

    /**
     * Gets x in range, returning 0 if x<min, max ist x > max, and x otherwise.
     * @param x position to check
     * @param max maximum allowed
     * @param min minimum allowed
     * @return 0 if x<min, max ist x > max and x otherwise.
     */
    int GetXInRange(int x, int min, int max);

    /**
     * Helper-funtion, checking whether given position is inside of the field.
     * @param pos
     * @return boolen indicating, whether given position is inside of field.
     */
    bool InField(position_t pos);

    int random_coordinate_shift(int x, int min, int max);

};


#endif
