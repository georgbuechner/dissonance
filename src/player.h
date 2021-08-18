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

typedef std::pair<int, int> Position;
typedef std::pair<double, bool> Resource;
typedef const std::map<int, double> Costs;

class Player {
  public:

    /**
     * Constructor initializing all resources and gatherers with defaul values
     * and creates den with given position.
     * @param[in] den position of this player's den.
     * @param[in] silver initial silver value.
     */
    Player(Position den_pos, int silver=0);

    // getter:
    std::map<std::string, Epsp> soldier();
    std::map<Position, Synapse> barracks();
    std::set<Position> units_and_buildings();
    Position den_pos();
    int cur_range();
    int iron();
    std::map<int, Resource> resources();

    // methods:

    void AddBuilding(Position pos);

    bool DamageSoldier(std::string id);

    
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

    bool DistributeIron(int resource);

    Costs CheckResources(int unit);

    /** 
     * Adds specified gatherer.
     * @param unit specify which gatherer
     */
    void AddGatherer(int unit);

    /**
     * Adds a new defence tower at the given position. 
     * @param[in] pos position at which to add defence tower.
     */
    void AddDefenceTower(Position pos);

    void AddBarrack(Position pos);

    /**
     * Adds new soldier and sets it's current position and the way to it's
     * target.
     * A soldier is always created with a new, random id, as (in contrast to
     * other units/ buildings) a soldier is not uniquely defined by it's
     * position.
     * @param[in] pos start position of new soldier.
     * @param[in] way to the enemies building.
     */
    void AddSoldier(Position pos, std::list<Position> way);

    /**
     * Moves every soldier forward and if it's target is reached, damage is
     * increased and the soldier is removed.
     * @param[in] enemy_den position of the enemies building.
     * @return damage inficted to the target.
     */
    int MoveSoldiers(Position enemy_den);

    /**
     * Remove a soldier if it exists.
     * @param[in] id of soldier which shall be removed.
     */
    void RemoveSoldier(std::string id);

    /**
     * Function checking whether a tower has defeted a soldier.
     * @param player 
     * @param ki_
     */
    void HandleDef(Player* enemy);

    /**
     * Checks if a soldier on the map belongs to this player.
     * @param[in] pos position to check for.
     */
    bool IsSoldier(Position pos);

    bool IsActivatedResource(int resource);

    /** 
     * Decrease live of den.
     * @param[in] val values to decrease life by.
     * @return whether building is destroied.
     */
    bool DecreaseDenLp(int val);

  private: 
    int cur_range_;
    int oxygen_boast_;
    double total_oxygen_;
    double bound_oxygen_;
    std::map<int, Resource> resources_;
    std::shared_mutex mutex_resources_;

    std::chrono::time_point<std::chrono::steady_clock> last_iron_; 



    Nucleus den_;
    std::shared_mutex mutex_den_;

    std::map<std::string, Epsp> soldiers_;
    std::shared_mutex mutex_soldiers_;

    std::set<Position> all_units_and_buildings_;  ///< simple set, to check whether position belongs to player.
    std::shared_mutex mutex_units_and_buildings_;

    std::map<Position, Synapse> barracks_;
    std::shared_mutex mutex_barracks;

    std::map<Position, ActivatedNeuron> defence_towers_;
    std::shared_mutex mutex_defence_towers_;

    // methods
    void TakeResources(Costs costs);
};

#endif
