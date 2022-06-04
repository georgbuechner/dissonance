#include "server/game/player/monto_carlo_ai.h"
#include "server/game/player/player.h"
#include "share/audio/audio.h"
#include "share/constants/codes.h"
#include "share/defines.h"
#include "share/tools/random/random.h"
#include "share/tools/utils/utils.h"
#include "spdlog/spdlog.h"
#include <deque>
#include <vector>

MontoCarloAi::MontoCarloAi(position_t nucleus_pos, Field* field, Audio* audio, RandomGenerator* ran_gen, int color) 
  : Player(nucleus_pos, field, ran_gen, color) {
  spdlog::get(LOGGER)->info("Creating MontoCarloAi...");
  audio_ = audio;
  ran_gen_ = new RandomGenerator();
  data_per_beat_ = audio_->analysed_data().data_per_beat_;
  last_action_ = 0;
  nucleus_pos_ = nucleus_pos;
}

MontoCarloAi::MontoCarloAi(const Player& ai, Field* field) : Player(ai, field) {
  audio_ = new Audio(*ai.audio());
  data_per_beat_ = ai.data_per_beat();
  last_data_point_ = ai.last_data_point();
  nucleus_pos_ = ai.nucleus_pos();
  spdlog::get(LOGGER)->debug("MontoCarloAi::MontoCarloAi. Added new monto_carlo_ai with inition neurons:");
  for (const auto& it : neurons_) {
    spdlog::get(LOGGER)->debug(" - {}: pos: {}, sym: {}", units_tech_name_mapping.at(it.second->type_), 
        utils::PositionToString(it.first), field->GetSymbolAtPos(it.first));
  }
}

MontoCarloAi::~MontoCarloAi() { delete audio_; }

// getter
Audio* MontoCarloAi::audio() const { return audio_; }
std::deque<AudioDataTimePoint> MontoCarloAi::data_per_beat() const { return data_per_beat_; }
AudioDataTimePoint MontoCarloAi::last_data_point() const { return last_data_point_; }
std::vector<Player::AiOption> MontoCarloAi::actions() const { return actions_; }
position_t MontoCarloAi::nucleus_pos() const { return nucleus_pos_; }
int MontoCarloAi::last_action() const { return last_action_; }

// methods
bool MontoCarloAi::DoAction() {
  spdlog::get(LOGGER)->debug("MontoCarloAi::GetchChoices (random).");
  auto next_data_at_beat = data_per_beat_.front();
  data_per_beat_.pop_front();
  // check whether to generate actions.
  // if (actions_.size() == 0 || last_action_++ == 10)
    GetchChoices();
  // get random action
  spdlog::get(LOGGER)->info("got {} actions", actions_.size());
  if (actions_.size() > 0) {
    auto action = actions_[ran_gen_->RandomInt(0, actions_.size()-1)];
    ExecuteAction(next_data_at_beat, action);
  }
  last_data_point_ = next_data_at_beat;
  // reload
  if (data_per_beat_.size() == 0) {
    data_per_beat_ = audio_->analysed_data().data_per_beat_;
    return true;
  }
  return false;
}

bool MontoCarloAi::DoGivenAction(AiOption action) {
  spdlog::get(LOGGER)->debug("MontoCarloAi::GetchChoices (given). action: {}", action._type);
  auto next_data_at_beat = data_per_beat_.front();
  data_per_beat_.pop_front();
  // check whether to generate actions.
  ExecuteAction(next_data_at_beat, action);
  last_data_point_ = next_data_at_beat;
  // reload
  if (data_per_beat_.size() == 0) {
    data_per_beat_ = audio_->analysed_data().data_per_beat_;
    return true;
  }
  return false;
}

void MontoCarloAi::ExecuteAction(const AudioDataTimePoint&, AiOption action) {
  spdlog::get(LOGGER)->info("MontoCarloAi::GetchChoices (main). action: {}", action.string());
  
  // units
  if (action._type == EPSP) {
    auto costs = units_costs_.at(UnitsTech::EPSP);
    size_t res = resources_.at(POTASSIUM).cur() / costs[POTASSIUM];
    for (int i=0; i < res*action._num; i++) {
      if (!AddPotential(action._pos, EPSP, i/2))
        break;
    }
  }
  else if (action._type == IPSP) {
    auto costs = units_costs_.at(UnitsTech::IPSP);
    size_t res = std::min(resources_.at(POTASSIUM).cur() / costs[POTASSIUM], 
        resources_.at(CHLORIDE).cur() / costs[CHLORIDE]);
    for (int i=0; i < res*action._num; i++) {
      if (!AddPotential(action._pos, IPSP, i/2))
        break;
    }
  }
  else if (action._type == MACRO) {
    AddPotential(action._pos, MACRO);
  }
  // neurons
  else if (action._type == ACTIVATEDNEURON) {
    // get random free position
    auto pos = field_->FindFree(action._pos, 1, cur_range_);
    if (pos == DEFAULT_POS)
      AddTechnology(UnitsTech::NUCLEUS_RANGE);
    else {
      spdlog::get(LOGGER)->debug("Adding ActivatedNeuron. Current symbol on field {}", field_->GetSymbolAtPos(pos));
      AddNeuron(pos, ACTIVATEDNEURON);
    }
  }
  else if (action._type == SYNAPSE) {
    auto pos = field_->FindFree(action._pos, 1, cur_range_);
    if (pos == DEFAULT_POS)
      AddTechnology(UnitsTech::NUCLEUS_RANGE);
    else {
      // get random-epsp/ipsp-target
      auto enemy_neurons = enemies_.front()->GetAllPositionsOfNeurons();
      auto ipsp_target = enemy_neurons[ran_gen_->RandomInt(0, enemy_neurons.size()-1)];
      auto epsp_target = enemies_.front()->GetOneNucleus();
      AddNeuron(pos, SYNAPSE, epsp_target, ipsp_target);
    }
  }
  else if (action._type == NUCLEUS) {
    auto pos = field_->FindFree(field_->GetAllCenterPositionsOfSections()[action._num], 1, 4);
    if (pos != DEFAULT_POS)
      AddNeuron(pos, NUCLEUS);
  }
  // resources 
  else if (action._type == 100) {
    DistributeIron(action._num);
  }
  else if (action._type == 150) {
    RemoveIron(action._num);
  }
  // Change target
  else if (action._type == 200) {
    ChangeEpspTargetForSynapse(action._pos, action._pos_2);
  }
  else if (action._type == 250) {
    ChangeEpspTargetForSynapse(action._pos, action._pos_2);
  }
}

std::vector<Player::AiOption> MontoCarloAi::GetchChoices() {
  spdlog::get(LOGGER)->debug("MontoCarloAi::GetchChoices.");
  actions_.clear();
  actions_.push_back(AiOption{-1, DEFAULT_POS, DEFAULT_POS, 0});
  if (resources_.at(IRON).cur() > 0) {
    unsigned int resources_activated = 0;
    for (const auto& it : resources_)
      resources_activated+=it.second.Active();
    for (const auto& it : resources_) {
      if (it.first == IRON)
        continue;
      if (it.second.distributed_iron() > 1 && it.second.cur() > 25 && resources_activated < resources_.size()) 
        actions_.push_back({150, DEFAULT_POS, DEFAULT_POS, (float)it.first});
      else if (it.second.distributed_iron() < 3 
          || (it.second.distributed_iron() < 5 && resources_activated == resources_.size()))
        actions_.push_back({100, DEFAULT_POS, DEFAULT_POS, (float)it.first});
    }
  }
  auto building_options = GetBuildingOptions();
  if (building_options[0]) {
    for (const auto& synapse_pos : GetAllPositionsOfNeurons(SYNAPSE)) {
      for (float num : {0.1, 0.5, 1.0})
        actions_.push_back(AiOption{IPSP, synapse_pos, DEFAULT_POS, num});
    }
  }
  if (building_options[1]) {
    for (const auto& synapse_pos : GetAllPositionsOfNeurons(SYNAPSE)) {
      for (float num : {0.1, 0.5, 1.0})
        actions_.push_back(AiOption{EPSP, synapse_pos, DEFAULT_POS, num});
    }
  }
  if (building_options[2]) {
    for (const auto& synapse_pos : GetAllPositionsOfNeurons(SYNAPSE)) {
      actions_.push_back(AiOption{MACRO, synapse_pos, DEFAULT_POS, -1});
    }
  }
  if (building_options[3] && GetAllPositionsOfNeurons(ACTIVATEDNEURON).size() < 7) {
    for (const auto& nucleus_pos : GetAllPositionsOfNeurons(NUCLEUS))
      actions_.push_back(AiOption{ACTIVATEDNEURON, nucleus_pos, DEFAULT_POS, -1});
  }
  if (building_options[4] && GetAllPositionsOfNeurons(SYNAPSE).size() < 4) {
    for (const auto& nucleus_pos : GetAllPositionsOfNeurons(NUCLEUS))
      actions_.push_back(AiOption{SYNAPSE, nucleus_pos, DEFAULT_POS, -1});
  }
  if (building_options[5] && GetAllPositionsOfNeurons(SYNAPSE).size() < 2) {
     for (int i=0; i<8; i++) 
       actions_.push_back(AiOption{NUCLEUS, DEFAULT_POS, DEFAULT_POS, (float)i});
  }
  if (GetAllPositionsOfNeurons(SYNAPSE).size() > 0) {
    for (const auto& synapse_pos : GetAllPositionsOfNeurons(SYNAPSE)) {
      for (const auto& target_pos : enemies_.front()->GetAllPositionsOfNeurons()) {
        if (ran_gen_->RandomInt(0, 2) == 1)
          actions_.push_back(AiOption{200, synapse_pos, target_pos, -1});
      }
      for (const auto& target_pos : enemies_.front()->GetAllPositionsOfNeurons(ACTIVATEDNEURON)) {
        if (ran_gen_->RandomInt(0, 2) == 1)
          actions_.push_back(AiOption{250, synapse_pos, target_pos, -1});
      }
    }
  }
  last_action_ = 0;
  // get random nucleus
  auto nucleus_positions = GetAllPositionsOfNeurons(UnitsTech::NUCLEUS);
  nucleus_pos_ = nucleus_positions[ran_gen_->RandomInt(0, nucleus_positions.size()-1)];
  spdlog::get(LOGGER)->debug("MontoCarloAi::GetchChoices. nucleus_pos_: {}", utils::PositionToString(nucleus_pos_));
  return actions_;
}
