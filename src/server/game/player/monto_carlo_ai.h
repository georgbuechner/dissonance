#ifndef SRC_PLAYER_MONTOCARLOAI_H_
#define SRC_PLAYER_MONTOCARLOAI_H_

#include <cstddef>
#include <vector>

#include "share/audio/audio.h"
#include "server/game/field/field.h"
#include "share/defines.h"
#include "share/objects/units.h"
#include "server/game/player/player.h"

class RandomChoiceAi : public Player {
  public:
    RandomChoiceAi(position_t nucleus_pos, Field* field, Audio* audio, RandomGenerator* ran_gen, int color);
    RandomChoiceAi(const Player& ai, Field* field);
    ~RandomChoiceAi();
    
    // getter 
    Audio* audio() const;
    std::deque<AudioDataTimePoint> data_per_beat() const;
    AudioDataTimePoint last_data_point() const;
    int action_pool() const;
    std::vector<AiOption> actions() const;
    int last_action() const;
    McNode* mc_node();

    // methods
    bool DoRandomAction();
    bool DoAction(AiOption action);
    std::vector<AiOption> GetchChoices();
    void SafeTree();

  private:
    // members 
    Audio* audio_;
    std::deque<AudioDataTimePoint> data_per_beat_;
    AudioDataTimePoint last_data_point_;
    bool delete_audio_;

    int action_pool_;
    std::vector<AiOption> actions_;
    int last_action_;

    McNode* mc_node_;

    // members functions
    void ExecuteAction(const AudioDataTimePoint& data_at_beat, AiOption action);
    void GetAvailibleActions();
};

#endif
