#ifndef SRC_PLAYER_AUDIOKI_H_
#define SRC_PLAYER_AUDIOKI_H_

#include "audio/audio.h"
#include "game/field.h"
#include "objects/units.h"
#include "player/player.h"
#include <cstddef>

class AudioKi : public Player {
  public:
    AudioKi(Position nucleus_pos, int iron, Player* player, Field* field, Audio* audio);

    void SetUpTactics(Interval interval);
    void DoAction(const AudioDataTimePoint& data_at_beat, bool level_switch);
    void HandleIron(const AudioDataTimePoint& data_at_beat);

  private:
    Player* player_;
    Audio* audio_;
    const float average_bpm_;
    const float average_level_;
    size_t max_activated_neurons_;
    Position nucleus_pos_;
    int intelligenz_;
    std::set<int> notes_played_;

    bool attack_focus_;
    std::map<int, int> attack_strategies_;
    std::map<int, int> defence_strategies_;
    std::map<int, int> technology_tactics_;
    std::map<int, int> resource_tactics_;
    std::map<int, int> building_tactics_;

    void CreateEpsp(const AudioDataTimePoint& data_at_beat);
    void CreateIpsp(const AudioDataTimePoint& data_at_beat);
    void CreateIpspThenEpsp(const AudioDataTimePoint& data_at_beat);
    void CreateSynapses(bool force=false);
    void CreateActivatedNeuron(bool force=false);
    void NewTechnology(const AudioDataTimePoint& data_at_beat);
    void KeepOxygenLow(const AudioDataTimePoint& data_at_beat);

    typedef std::list<std::pair<size_t, size_t>> sorted_stragety;
    sorted_stragety SortStrategy(std::map<int, int> strategy);
};

#endif
