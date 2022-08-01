#ifndef SRC_PLAYER_AUDIOKI_H_
#define SRC_PLAYER_AUDIOKI_H_

#include <cstddef>
#include <deque>
#include <queue>
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
    std::map<std::string, size_t> strategies() const {
      std::map<std::string, size_t> strategies;
      strategies["Epsp target strategy: "] = epsp_target_strategy_;
      strategies["Ipsp target strategy: "] = ipsp_target_strategy_;
      strategies["Activated neuron strategy: "] = activated_neuron_strategy_;
      strategies["Main defence strategy: "] = activated_neuron_strategy_;
      return strategies;
    }
    
    void SetUpTactics(bool economy_tactics);
    bool DoAction();
    void HandleIron(const AudioDataTimePoint& data_at_beat);

  private:
    // members
    Audio* audio_;
    std::deque<AudioDataTimePoint> data_per_beat_;
    const float average_bpm_;
    const float average_level_;
    size_t max_activated_neurons_;
    position_t nucleus_pos_;

    AudioDataTimePoint last_data_point_;
    Interval cur_interval_;
    std::vector<AudioDataTimePoint> last_data_points_above_average_level_;

    std::map<size_t, size_t> attack_strategies_;
    std::map<size_t, size_t> defence_strategies_;
    std::vector<size_t> resource_tactics_;
    std::set<size_t> resources_activated_;
    std::vector<size_t> technology_tactics_;
    std::map<size_t, size_t> building_tactics_;
    size_t epsp_target_strategy_;
    size_t ipsp_target_strategy_;
    size_t activated_neuron_strategy_;
    size_t main_def_strategy_;
    std::map<unsigned int, unsigned int> extra_activated_neurons_;

    // functions 
    bool DoAction(const AudioDataTimePoint& data_at_beat);

    void LaunchAttack(const AudioDataTimePoint& data_at_beat);

    // Create potental/ neurons. Add technology
    void CreateEpsps(position_t synapse_pos, position_t target_pos, int inital_speed_decrease);
    void CreateIpsps(position_t synapse_pos, position_t target_pos, int num_ipsp_to_create);
    void CreateIpspThenEpsp(const AudioDataTimePoint& data_at_beat);
    void CreateSynapses(bool force=false);
    void CreateActivatedNeuron(bool force=false);
    void NewTechnology(const AudioDataTimePoint& data_at_beat);


    size_t AvailibleIpsps();
    size_t AvailibleEpsps(size_t ipsps_to_create);
    std::vector<position_t> AvailibleIpspLaunches(std::vector<position_t>& synapses, int min);
    size_t GetLaunchAttack(const AudioDataTimePoint& data_at_beat, size_t ipsps_to_create);

    std::vector<position_t> GetEpspTargets(position_t synapse_pos, std::list<position_t> way, size_t ignore_strategy=-1);
    std::vector<position_t> GetIpspTargets(std::list<position_t> way, std::vector<position_t>& synapses, 
        size_t ignore_strategy=-1);

    // Other stretegies

    /**
     * Checks if resource limit is reached.
     * If for any resource the limit is above 80%, try to research TOTAL_RESOURCE. TODO (fux): or build nucleus.
     */
    void CheckResourceLimit();
    /**
     * Gets called to add defencive structures, if enemy has potentals.
     * Calls `IpspDef` or `CreateExtraActivatedNeurons`, depending on
     * strategies.
     */
    void Defend();

    /**
     * Launches Ipsp in the direction of enemy potentials.
     * @param[in] enemy_potentials (number of incoming enemy epsps)
     * @param[in] way (way of random enemy incoming epsp)
     * @return false if defence-strategy could not be applied, true otherwise.
     */
    bool IpspDef(unsigned int enemy_potentials, std::list<position_t> way, int diff);

    /**
     * Creates extra activated neuerons based on enemies attack-strength.
     * @param[in] enemy_potentials (number of incoming enemy epsps)
     * @param[in] way (way of random enemy incoming epsp)
     * @return false if defence-strategy could not be applied, true otherwise.
     */
    bool CreateExtraActivatedNeurons(unsigned int enemy_potentials, std::list<position_t> way, int diff);

    // helpers
    typedef std::list<std::pair<size_t, size_t>> sorted_stragety;
    sorted_stragety SortStrategy(std::map<size_t, size_t> strategy);
    std::vector<position_t> GetAllActivatedNeuronsOnWay(std::vector<position_t> neurons, std::list<position_t> way);
    std::vector<position_t> SortPositionsByDistance(position_t start, std::vector<position_t> positions, bool reverse=false);
    std::vector<position_t> GetEnemySynapsesSortedByLeastDef(position_t start);
    size_t GetMaxLevelExeedance() const;
    int SynchAttacks(size_t epsp_way_length, size_t ipsp_way_length);

    void SetBattleTactics();
    void SetEconomyTactics();
};

#endif
