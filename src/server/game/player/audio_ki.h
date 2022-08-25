#ifndef SRC_PLAYER_AUDIOKI_H_
#define SRC_PLAYER_AUDIOKI_H_

#include <cstddef>
#include <deque>
#include <queue>
#include <shared_mutex>
#include <vector>

#include "share/audio/audio.h"
#include "server/game/field/field.h"
#include "share/defines.h"
#include "share/objects/units.h"
#include "server/game/player/player.h"

class AudioKi : public Player {
  public:
    AudioKi(position_t nucleus_pos, Field* field, Audio* audio, RandomGenerator* ran_gen, int color);
    ~AudioKi() {};

    // getter 
    std::deque<AudioDataTimePoint> data_per_beat() const;
    std::map<std::string, size_t> strategies() const;
    
    /**
     * Sets up tactics. Always sets up battle-tactics, economical tactics only
     * on first call.
     * @param[in] economy_tactics if true, sets up economical tactics too.
     */
    void SetUpTactics(bool economy_tactics);

    /**
     * Executes ai-action.
     * Gets next beat and calls `DoAction(next_beat)`. Then sets last_beat_
     * @returns true if last-beat is reached. Falls otherwise.
     */
    bool DoAction();

    /**
     * Distributes iron to a resource. 
     * Uses order defined by tactics. If iron comes in slowly, potentially
     * forwards non-active resource, if an active resource is plentiful.
     */
    void HandleIron();

  private:
    // members
    position_t nucleus_pos_;

    // audio-data
    Audio* audio_;
    std::deque<AudioDataTimePoint> audio_beats_;
    Interval cur_interval_;
    const float average_level_;
    AudioDataTimePoint last_beat_;
    std::vector<size_t> last_level_peaks_;

    // tactics
    std::map<size_t, size_t> epsp_target_strategies_;  // DESTROY nucleus/activatedneurons/synapses/resources
    std::map<size_t, size_t> ipsp_target_strategies_;  // BLOCK activatedneurons/synapses/resources
    std::map<size_t, size_t> ipsp_epsp_strategies_; // front-/ surround-focus
    std::map<size_t, size_t> activated_neuron_strategies_; // front-/ surround-focus
    std::map<size_t, size_t> def_strategies_; // front-/ surround-focus
    std::vector<size_t> resource_tactics_;
    std::set<size_t> resources_activated_;
    std::vector<size_t> technology_tactics_;
    std::map<size_t, size_t> building_tactics_;

    // functions 

    /**
     * Does ai-action
     * Audio-dependant actions:
     * - On interval change: reload battle-tactics
     * - level rises above average: build synapse
     * - peak is over: launch attack 
     * - level falls below average: build activated neuron
     * - more off-notes: researches new technology
     * - otherwise: distributes iron
     * Audio-independent actions:
     * - rebuilds destroyed resource-neurons 
     * - handles high resource bounds/ limits
     * - checks whether (additional) defence is needed.
     */
    bool DoAction(const AudioDataTimePoint& beat);

    /**
     * Coordinates attack of ipsps and epsps.
     */
    void LaunchAttack();

    /**
     * Tries to launch given number of epsps from given synapse (sets target for this synapse)
     * If speed_decrease is set, adds initial-speed decrease (to synch attack with ipsps)
     * @param[in] synapse_pos
     * @param[in] target_pos 
     * @param[in] num_epsps_to_create
     * @param[in] speed_decrease
     */
    void CreateEpsps(position_t synapse_pos, position_t target_pos, int num_epsps_to_create, int speed_decrease);

    /**
     * Tries to launch given number of ipsps from given synapse (sets target for this synapse)
     * @param[in] synapse_pos
     * @param[in] target_pos 
     * @param[in] num_ipsps_to_create
     */
    void CreateIpsps(position_t synapse_pos, position_t target_pos, int num_ipsp_to_create);

    /**
     * Creates synapse, while considering resource limits.
     */
    void CreateSynapses();

    /**
     * Creates activated neuron, while considering resource limits if not force.
     * @param[in] force
     */
    void CreateActivatedNeuron(bool force=false);

    /**
     * Researches new technology using order defined by tactics. 
     */
    void NewTechnology();
    
    // Additional strategies

    /**
     * Gets called to add defensive structures, if enemy has potentials.
     * Calls `IpspDef` or `CreateExtraActivatedNeurons`, depending on strategies.
     */
    void Defend();

    /**
     * Launches ipsp in the direction of enemy potentials.
     * @param[in] enemy_potentials (number of incoming enemy epsps)
     * @param[in] way (way of random enemy incoming epsp)
     * @return false if defence-strategy could not be applied, true otherwise.
     */
    bool IpspDef(unsigned int enemy_potentials, std::list<position_t> way, int diff);

    /**
     * Creates extra activated neurons based on enemies attack-strength.
     * @param[in] enemy_potentials (number of incoming enemy epsps)
     * @param[in] way (way of random enemy incoming epsp)
     * @return false if defence-strategy could not be applied, true otherwise.
     */
    bool CreateExtraActivatedNeurons(unsigned int enemy_potentials, const std::list<position_t>& way, int diff);

    /**
     * Fixes high bounds on resources.
     * Finds resource with highest bound (>70), while prioritising oxygen (>65).
     * First solution: increase resource-bounds.
     * Second solution: distribute all iron but on to given resource.
     * Last solution: try to destroy synapse.
     */
    void HandleHighBound();

    
    // helpers-for launching

    /** 
     * Gets number of available ipsps depending on current resources.
     * @return number of available ipsps.
     */
    size_t AvailibleIpsps() const;

    /**
     * Gets number of available ipsps depending on current resources and number
     * of ipsps to create (assures ipsps can be created).
     * @param[in] num_ipsps_to_create
     * @return number of available epsps.
     */
    size_t AvailibleEpsps(size_t num_ipsps_to_create) const;

    /**
     * Gets number of epsps to create depending on current peak, enemy activated-neurons 
     * and number of ipsps to create.
     * @param[in] epsp_way
     * @param[in] num_ipsps_to_create
     * @return number of epsps to create.
     */
    size_t NumberOfEpspsToCreate(const std::list<position_t>& epsp_way, size_t num_ipsps_to_create) const;

    typedef std::pair<position_t, position_t> launch_target_pair_t;

    /**
     * Gets epsp-target. 
     * Favors target, which is not already ipsp-target.
     * @param[in] synapse_pos
     * @param[in] way 
     * @param[in] ipsp_launch_target_pairs (used to get ipsp-targets)
     * @return single epsp-target.
     */
    position_t GetEpspTarget(position_t synapse_pos, const std::list<position_t>& way, 
        const std::vector<launch_target_pair_t>& ipsp_launch_target_pairs) const;

    /**
     * Gets list of epsp-targets based on strategy (enemies activated-neurons, synapses,
     * resource-neurons or nucleus).
     * If no targets where found with a given strategy, retry ignoring this
     * strategy.
     * @param[in] synapse_pos
     * @param[in] way 
     * @param[in] ignore strategy (used if f.e.~no targets for first attempt where found.
     * @return list of epsp-targets 
     */
    std::vector<position_t> GetEpspTargets(position_t synapse_pos, const std::list<position_t>& way, 
        size_t ignore_strategy=-1) const;

    /**
     * Gets positions of synapses to use as ipsp-launches.
     * @param[in] synapses (synapses sorted by least distance to enemy nucleus).
     * @param[in] ipsp_per_synapse (number of ipsps at least to launch from one synapse)
     * @return sorted list of synapse-positions.
     */
    std::vector<position_t> AvailibleIpspLaunches(const std::vector<position_t>& synapses, int ipsp_per_synapse) const;

    /**
     * Gets array of ipsp-launch-synapses and ipsp-targets depending on
     * strategy. Size of array =min(available-ipsps-launch-synapse, num-enemy-neurons-to-target)
     * @param[in] ignore_strategy
     */
    std::vector<launch_target_pair_t> GetIpspLaunchesAndTargets(const std::list<position_t>& way, 
        const std::vector<position_t>& synapses, size_t ignore_strategy=-1) const;

    
    // helpers
    typedef std::list<std::pair<size_t, size_t>> sorted_stragety; ///< value -> strategy
    /**
     * Sorts strategy as list of {score, strategy}, where first entry has
     * highest score.
     * @param[in] strategy to sort
     * @returns sorted strategy (`[{score, strategy}, ...]`).
     */
    static sorted_stragety SortStrategy(const std::map<size_t, size_t>& strategy);

    /**
     * Gets strategy with highest score.
     * @param[in] strategy 
     * @return strategy with highest score.
     */
    static size_t GetTopStrategy(const std::map<size_t, size_t>& strategy);

    /**
     * From given list of activated neurons, gets all activated neurons on given
     * way.
     * @param[in] activated neurons
     * @param[in] way 
     * @return list of activated neurons on given way.
     */
    static std::vector<position_t> GetAllActivatedNeuronsOnWay(std::vector<position_t> activated_neurons, 
        const std::list<position_t>& way);

    /**
     * Sorts given list of positions by distance to given position.
     * @param[in] start
     * @param[in] positions
     * @param returns list of positions sorted by distance to given position.
     */
    static std::vector<position_t> SortPositionsByDistance(position_t start, const std::vector<position_t>& positions);

    /**
     * Gets enemy neurons of given type by how strongly defended by activated neurons.
     * Should be used on either SYNAPSE, or RESOURCENEURON.
     * @param[in] start (position from which potential starts)
     * @param[in] neuron_type 
     * @return gets neurons sorted by least defended (by enemies activated neurons)
     */
    std::vector<position_t> GetEnemyNeuronsSortedByLeastDef(position_t start, int neuron_type) const;

    /**
     * Gets percentage of peak above average, since last increase, in relation
     * to max-peak.
     * This determines the number of epsp to create.
     * @return percent (0-1) of `cur_max_peak/max_peak`
     */
    double GetMaxLevelExeedance() const;

    /**
     * Gets number of cycles to delay epsp-launch by to synchronise epsp and ipsp
     * arrival at target.
     * @param[in] epsp_way_length
     * @param[in] ipsp_way_length
     * @return necessary speed decrease.
     */
    int SynchAttacks(size_t epsp_way_length, size_t ipsp_way_length) const;

    /**
     * Gets voltage of nucleus nearest to enemy target.
     * @param[in] enemy_target_pos
     * @return voltage, -1 if no nucleus was found.
     */
    int GetVoltageOfAttackedNucleus(position_t enemy_target_pos);

    /**
     * Extraordinarily increases next resource, if iron is low, but an active resource is plentiful.
     * Deactives highest resource (>40) and uses iron, to active next resource. 
     * Put's deactivated resource as next in line. 
     * Does not do anything if next resource is also plentiful.
     * @return true took action, false otherwise.
     */
    bool LowIronResourceDistribution();

    /**
     * Finds alternative way-points to beats bypass enemies defence.
     * @param[in] synapse_pos 
     * @param target_pos 
     * @return new waypoints if waypoints where found.
     */
    std::vector<position_t> FindBestWayPoints(position_t synapse_pos, position_t target_pos);

    /**
     * Sets up battle-tactics.
     */
    void SetBattleTactics();

    /**
     * Sets up economy-tactics.
     */
    void SetEconomyTactics();
};

#endif
