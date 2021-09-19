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

#include "player/player.h"
#include "utils/graph.h"
#include "objects/units.h"

class Field {
  public:
    /** Constructor initializing field with availible lines and columns
     * @param[in] lines availible lines.
     * @param[in] cols availible cols
     */
    Field(int lines, int cols);

    // getter:
    int lines();
    int cols();
    std::vector<Position> highlight();

    // setter:
    void set_highlight(std::vector<Position> positions);
    void set_range(int);
    void set_range_center(Position pos);
    void set_replace(std::map<Position, char> replacements);

    // methods:

    /**
     * Adds random natural barriers.
     */
    void AddHills();

    /**
     * Adds position for player den and random position for player resources.
     * @param[in] min_line
     * @param[in] max_line
     * @param[in] min_col
     * @param[in] max_col
     * @return position of player den.
     */
    Position AddDen(int min_line, int max_line, int min_col, int max_col);

    /**
     * Builds graph to calculate ways.
     * Function should be called after field is initialized. Function checks all
     * fields reachable from player-den (this could be the den of any player).
     * @param[in] player_den position of player's den.
     * @param[in] enemy_den position of enemy's den.
     */
    void BuildGraph(Position player_den, Position enemy_den);

    /**
     * Finds a free position for a new soldier.
     * @param pos position around which to add soldier.
     * @return position of new soldier.
     */
    Position GetNewSoldierPos(Position pos);

    /**
     * Finds a free position for a defence tower and adds tower to player/ ki
     * units.
     * @param pos position of new defence tower.
     * @param unit integer to identify unit.
     */
    void AddNewUnitToPos(Position pos, int unit);


    /** 
     * Prints current field. 
     * Updates the field stacking soldiers.
     * @param player pointer to the player.
     * @param ki pointer to the ki
     * @param show_in_graph if set to true, highlights all free positions in the
     * way between player 1 and player 2 den.
     */
    void PrintField(Player* player, Player* ki);

    /**
     * Gets way to a soldiers target.
     * @param start_pos starting position.
     * @param target_pos target position.
     * @return list of positions.
     */ 
    std::list<Position> GetWayForSoldier(Position start_pos, std::vector<Position> way_points);

    /** 
     * Finds the next free position near a given position with min and max
     * deviation.
     * @param l initial line
     * @param c initial column.
     * @param min distance 
     * @param max distance
     * @return free position in given range.
     */
    Position FindFree(int l, int c, int min, int max);

    bool IsFree(Position pos);

    bool InRange(Position pos, int range, Position start = {-1, -1});

    Position GetSelected(char replace, int num);

  private: 
    int lines_;
    int cols_;
    Graph graph_;
    std::vector<std::vector<std::string>> field_;
    std::shared_mutex mutex_field_;

    std::vector<Position> highlight_;
    volatile int range_;
    Position range_center_;
    std::map<Position, char> replacements_;

    /** 
     * Adds resources ([G]old, [S]ilver, [B]ronze) near given position.
     * @param start_pos position near which to create resources.
     */
    void AddResources(Position start_pos);

    /**
     * Places moving units (soldiers) on the field, by taking units from given
     * player. 
     * Usually this function is called for each player just before printing the
     * field. This function only modifies a temporary copy of the field.
     * @param[in] player player of which to add soldiers.
     * @param[out] field reference to copy of field.
     */
    void UpdateField(Player* player, std::vector<std::vector<std::string>>& field);

    bool CheckCollidingPotentials(Position pos, Player* player_one, Player* player_two);

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
     * @param l line
     * @param c column
     * @return boolen indicating, whether given position is inside of field.
     */
    bool InField(int l, int c);

    /**
     * Function clearing field (set positions to FREE) in a certain radius around 
     * a given position.
     * @param pos position which surrounding to clear.
     */ 
    void ClearField(Position pos);
};


#endif
