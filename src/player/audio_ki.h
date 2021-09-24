#ifndef SRC_PLAYER_AUDIOKI_H_
#define SRC_PLAYER_AUDIOKI_H_

#include "game/field.h"
#include "objects/units.h"
#include "player/player.h"
#include <cstddef>
#include <vector>

class AudioKi : public Player {
  public:
    AudioKi(Position nucleus_pos, int iron, Field* field, Audio* audio);

    // getter 
    
    // setter
    void set_last_time_point(const AudioDataTimePoint& data_at_beat);

    void SetUpTactics();
    void DoAction(const AudioDataTimePoint& data_at_beat);
    void HandleIron(const AudioDataTimePoint& data_at_beat);

  private:
    const float average_bpm_;
    const float average_level_;
    size_t max_activated_neurons_;
    Position nucleus_pos_;
    int intelligenz_;
    std::set<int> notes_played_;

    AudioDataTimePoint last_data_point_;
    Interval cur_interval_;

    std::vector<AudioDataTimePoint> last_data_points_above_average_level_;

    bool attack_focus_;
    std::map<size_t, size_t> attack_strategies_;
    std::map<size_t, size_t> defence_strategies_;
    std::map<size_t, size_t> technology_tactics_;
    std::map<size_t, size_t> resource_tactics_;
    std::map<size_t, size_t> building_tactics_;

    size_t balancing_;
    size_t epsp_target_strategy_;
    size_t ipsp_target_strategy_;

    std::map<unsigned int, unsigned int> extra_nucleus_;

    void LaunchAttack(const AudioDataTimePoint& data_at_beat);

    void CreateEpsps(Position synapse_pos, Position target_pos, int bpm);
    void CreateIpsps(Position synapse_pos, Position target_pos, int num_ipsp_to_create, int bpm);
    void CreateIpspThenEpsp(const AudioDataTimePoint& data_at_beat);
    void CreateSynapses(bool force=false);
    void CreateActivatedNeuron(bool force=false);
    void NewTechnology(const AudioDataTimePoint& data_at_beat);
    void KeepOxygenLow(const AudioDataTimePoint& data_at_beat);

    typedef std::list<std::pair<size_t, size_t>> sorted_stragety;
    sorted_stragety SortStrategy(std::map<size_t, size_t> strategy);

    size_t AvailibleIpsps();
    size_t AvailibleEpsps(size_t ipsps_to_create);
    std::vector<Position> AvailibleIpspLaunches(std::vector<Position>& synapses, int min);
    size_t GetLaunchAttack(const AudioDataTimePoint& data_at_beat, size_t ipsps_to_create);

    std::vector<Position> GetEpspTargets(Position synapse_pos, std::list<Position> way, size_t ignore_strategy=-1);
    std::vector<Position> GetIpspTargets(std::list<Position> way, std::vector<Position>& synapses, 
        size_t ignore_strategy=-1);

    std::vector<Position> GetAllActivatedNeuronsOnWay(std::list<Position> way);
    std::vector<Position> SortPositionsByDistance(Position start, std::vector<Position> positions, bool reverse=false);
    std::vector<Position> GetEnemySynapsesSortedByLeastDef(Position start);
    size_t GetMaxLevelExeedance() const;

    void SynchAttacks(size_t epsp_way_length, size_t ipsp_way_length);
};

#endif
