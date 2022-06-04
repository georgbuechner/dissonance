#ifndef SRC_PLAYER_MONTOCARLOAI_H_
#define SRC_PLAYER_MONTOCARLOAI_H_

#include <cstddef>
#include <vector>

#include "share/audio/audio.h"
#include "server/game/field/field.h"
#include "share/defines.h"
#include "share/objects/units.h"
#include "server/game/player/player.h"

class MontoCarloAi : public Player {
  public:
    MontoCarloAi(position_t nucleus_pos, Field* field, Audio* audio, RandomGenerator* ran_gen, int color);
    MontoCarloAi(const Player& ai, Field* field);
    ~MontoCarloAi();
    
    // getter 
    Audio* audio() const;
    std::deque<AudioDataTimePoint> data_per_beat() const;
    AudioDataTimePoint last_data_point() const;
    std::vector<AiOption> actions() const;
    position_t nucleus_pos() const;
    int last_action() const;

    // methods
    bool DoAction();
    bool DoGivenAction(AiOption action);
    std::vector<AiOption> GetchChoices();

  private:
    // members 
    Audio* audio_;
    std::deque<AudioDataTimePoint> data_per_beat_;
    AudioDataTimePoint last_data_point_;

    std::vector<AiOption> actions_;
    position_t nucleus_pos_;
    int last_action_;

    // members functions
    void ExecuteAction(const AudioDataTimePoint& data_at_beat, AiOption action);
};

#endif
