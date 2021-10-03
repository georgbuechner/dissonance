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
#include "audio/audio.h"

class Field {
  public:
    /** Constructor initializing field with availible lines and columns
     * @param[in] lines availible lines.
     * @param[in] cols availible cols
     */
    Field(int lines, int cols, int left_border=0, Audio* audio=nullptr);

    // getter:
    int lines();
    int cols();
    std::vector<position_t> highlight();

    // setter:
    void set_highlight(std::vector<position_t> positions);
    void set_range(int);
    void set_range_center(position_t pos);
    void set_replace(std::map<position_t, char> replacements);

    // methods:

    /**
     * Adds random natural barriers.
     */
    void AddHills();

    /**
     * Adds position for player den and random position for player resources.
     * @param[in] section
     * @return position of player den.
     */
    position_t AddNucleus(int section);

    /**
     * Builds graph to calculate ways.
     * Function should be called after field is initialized. Function checks all
     * fields reachable from player-den (this could be the den of any player).
     * @param[in] player_den position of player's den.
     * @param[in] enemy_den position of enemy's den.
     */
    void BuildGraph(position_t player_den, position_t enemy_den);

    /**
     * Finds a free position for a new soldier.
     * @param pos position around which to add soldier.
     * @return position of new soldier.
     */
    position_t GetNewSoldierPos(position_t pos);

    /**
     * Finds a free position for a defence tower and adds tower to player/ ki
     * units.
     * @param pos position of new defence tower.
     * @param unit integer to identify unit.
     */
    void AddNewUnitToPos(position_t pos, int unit);

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
    std::list<position_t> GetWayForSoldier(position_t start_pos, std::vector<position_t> way_points);

    /** 
     * Finds the next free position near a given position with min and max
     * deviation.
     * @param l initial line
     * @param c initial column.
     * @param min distance 
     * @param max distance
     * @return free position in given range.
     */
    position_t FindFree(int l, int c, int min, int max);

    bool IsFree(position_t pos);

    bool InRange(position_t pos, int range, position_t start = {-1, -1});

    position_t GetSelected(char replace, int num);

    std::vector<position_t> GetAllInRange(position_t start, double max_dist, double min_dist, bool free=false);

    std::vector<position_t> GetAllCenterPositionsOfSections();
    std::vector<position_t> GetAllPositionsOfSection(unsigned int interval);

  private: 
    int left_border_;
    int lines_;
    int cols_;
    Audio* audio_;
    Graph graph_;
    std::vector<std::vector<std::string>> field_;
    std::shared_mutex mutex_field_;

    std::vector<position_t> highlight_;
    volatile int range_;
    position_t range_center_;
    std::map<position_t, char> replacements_;

    /** 
     * Adds resources ([G]old, [S]ilver, [B]ronze) near given position.
     * @param start_pos position near which to create resources.
     */
    void AddResources(position_t start_pos);

    /**
     * Places moving units (soldiers) on the field, by taking units from given
     * player. 
     * Usually this function is called for each player just before printing the
     * field. This function only modifies a temporary copy of the field.
     * @param[in] player player of which to add soldiers.
     * @param[out] field reference to copy of field.
     */
    void UpdateField(Player* player, std::vector<std::vector<std::string>>& field);

    bool CheckCollidingPotentials(position_t pos, Player* player_one, Player* player_two);

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

    int getrandom_int(int min, int max);
    int random_coordinate_shift(int x, int min, int max);

};


#endif
