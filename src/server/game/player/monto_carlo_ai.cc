#include "server/game/player/monto_carlo_ai.h"
#include "curses.h"
#include "nlohmann/json_fwd.hpp"
#include "server/game/player/player.h"
#include "share/audio/audio.h"
#include "share/constants/codes.h"
#include "share/defines.h"
#include "share/tools/random/random.h"
#include "share/tools/utils/utils.h"
#include "spdlog/spdlog.h"
#include <deque>
#include <unistd.h>
#include <vector>

RandomChoiceAi::RandomChoiceAi(position_t nucleus_pos, Field* field, Audio* audio, RandomGenerator* ran_gen, int color)
  : Player(nucleus_pos, field, ran_gen, color) {
  spdlog::get(LOGGER)->info("Creating MontoCarloAi...audio is {}", audio == nullptr);
  audio_ = audio;
  delete_audio_ = false;
  ran_gen_ = new RandomGenerator();
  data_per_beat_ = audio_->analysed_data().data_per_beat_;
  last_action_ = 0;
  std::cout << "data points: " << data_per_beat_.size() << std::endl;
  action_pool_ = data_per_beat_.size()/10;
  std::cout << "action_pool_ is " << action_pool_ << std::endl;
  mc_node_ = new McNode({0, 0.0});
}

RandomChoiceAi::RandomChoiceAi(const Player& ai, Field* field) : Player(ai, field) {
  spdlog::get(LOGGER)->debug("RandomChoiceAi::RandomChoiceAi: copy-constructor.");
  spdlog::get(LOGGER)->debug("RandomChoiceAi::RandomChoiceAi: creating audio. audio is nullptr? {}", 
      ai.audio() == nullptr);
  audio_ = new Audio(*ai.audio());
  spdlog::get(LOGGER)->debug("RandomChoiceAi::RandomChoiceAi: audio created.");
  delete_audio_ = true;
  data_per_beat_ = ai.data_per_beat();
  spdlog::get(LOGGER)->debug("RandomChoiceAi::RandomChoiceAi: data per beat set.");
  last_data_point_ = ai.last_data_point();
  spdlog::get(LOGGER)->debug("RandomChoiceAi::RandomChoiceAi: last data-point set.");
  action_pool_ = ai.action_pool();
  spdlog::get(LOGGER)->debug("RandomChoiceAi::RandomChoiceAi: action point set.");
  spdlog::get(LOGGER)->debug("MontoCarloAi::MontoCarloAi. Added new monto_carlo_ai with inition neurons:");
  for (const auto& it : neurons_) {
    spdlog::get(LOGGER)->debug(" - {}: pos: {}, sym: {}", units_tech_name_mapping.at(it.second->type_), 
        utils::PositionToString(it.first), field->GetSymbolAtPos(it.first));
  }
  spdlog::get(LOGGER)->debug("RandomChoiceAi::RandomChoiceAi: copy-constructor done.");
}

RandomChoiceAi::~RandomChoiceAi() { 
  if (delete_audio_)
    delete audio_; 
}

// getter
Audio* RandomChoiceAi::audio() const { return audio_; }
std::deque<AudioDataTimePoint> RandomChoiceAi::data_per_beat() const { return data_per_beat_; }
AudioDataTimePoint RandomChoiceAi::last_data_point() const { return last_data_point_; }
int RandomChoiceAi::action_pool() const { return action_pool_; }
std::vector<Player::AiOption> RandomChoiceAi::actions() const { return actions_; }
int RandomChoiceAi::last_action() const { return last_action_; }
Player::McNode* RandomChoiceAi::mc_node() { return mc_node_; }

// methods
bool RandomChoiceAi::DoRandomAction() {
  spdlog::get(LOGGER)->debug("MontoCarloAi::DoAction (random).");
  auto next_data_at_beat = data_per_beat_.front();
  data_per_beat_.pop_front();
  // Re-generate actions if no actions availibe or actions where not regenerated more than 10 times.
  // if (actions_.size() <= 1 || last_action_++ >= 10)
  GetchChoices();
  // get random action
  spdlog::get(LOGGER)->info("MontoCarloAi::DoAction: got {} actions", actions_.size());
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

bool RandomChoiceAi::DoAction(AiOption action) {
  spdlog::get(LOGGER)->debug("MontoCarloAi::DoGivenAction. action: {}", action._type);
  spdlog::get(LOGGER)->info("MontoCarloAi::DoGivenAction: got {} actions", actions_.size());
  auto next_data_at_beat = data_per_beat_.front();
  data_per_beat_.pop_front();
  ExecuteAction(next_data_at_beat, action);
  last_data_point_ = next_data_at_beat;
  // reload
  if (data_per_beat_.size() == 0) {
    data_per_beat_ = audio_->analysed_data().data_per_beat_;
    return true;
  }
  return false;
}

void RandomChoiceAi::ExecuteAction(const AudioDataTimePoint&, AiOption action) {
  spdlog::get(LOGGER)->info("MontoCarloAi::ExecuteAction. action: {}", action.string());
  // Decrease action-pool
  if (action._type != -1)
    action_pool_--;

  // units
  if (action._type == EPSP) {
    spdlog::get(LOGGER)->debug("RandomChoiceAi::ExecuteAction: adding epsp @{}. Num synapses: {}", 
        utils::PositionToString(action._pos), GetAllPositionsOfNeurons(SYNAPSE).size());
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
      spdlog::get(LOGGER)->debug("Adding Synapse: ipsp_target: {}, epsp_target: {}",
          utils::PositionToString(ipsp_target), utils::PositionToString(epsp_target));
      if (epsp_target == DEFAULT_POS || ipsp_target == DEFAULT_POS)
        spdlog::get(LOGGER)->debug("RandomChoiceAi::ExecuteAction Adding Synapse: enemy has no nucleus!");
      else
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
    // Distribute again if iron left and not actiaved yet.
    if (!resources_.at(action._num).Active() && resources_.at(IRON).cur() > 0)
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
  spdlog::get(LOGGER)->info("MontoCarloAi::ExecuteAction done.");
}

std::vector<Player::AiOption> RandomChoiceAi::GetchChoices() {
  spdlog::get(LOGGER)->debug("MontoCarloAi::GetchChoices.");
  spdlog::get(LOGGER)->info("MontoCarloAi::GetchChoices: action_pool: {}", action_pool_);
  actions_.clear();
  // Always add no-action as action.
  actions_.push_back(AiOption(-1, DEFAULT_POS, DEFAULT_POS, 0));
  // Only add other actions if action-pool is still "full"
  if (action_pool_ > 0) {
    GetAvailibleActions();
    spdlog::get(LOGGER)->info("MontoCarloAi::GetchChoices: actions {}", actions_.size());
  }
  last_action_ = 0;
  // get random nucleus
  return actions_;
}

void RandomChoiceAi::GetAvailibleActions() {
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
        actions_.push_back(AiOption(IPSP, synapse_pos, DEFAULT_POS, num));
    }
  }
  if (building_options[1]) {
    for (const auto& synapse_pos : GetAllPositionsOfNeurons(SYNAPSE)) {
      for (float num : {0.1, 0.5, 1.0})
        actions_.push_back(AiOption(EPSP, synapse_pos, DEFAULT_POS, num));
    }
  }
  if (building_options[2]) {
    for (const auto& synapse_pos : GetAllPositionsOfNeurons(SYNAPSE)) {
      actions_.push_back(AiOption(MACRO, synapse_pos, DEFAULT_POS, -1));
    }
  }
  if (building_options[3] && GetAllPositionsOfNeurons(ACTIVATEDNEURON).size() < 7) {
    for (const auto& nucleus_pos : GetAllPositionsOfNeurons(NUCLEUS))
      actions_.push_back(AiOption(ACTIVATEDNEURON, nucleus_pos, DEFAULT_POS, -1));
  }
  if (building_options[4] && GetAllPositionsOfNeurons(SYNAPSE).size() < 4) {
    for (const auto& nucleus_pos : GetAllPositionsOfNeurons(NUCLEUS))
      actions_.push_back(AiOption(SYNAPSE, nucleus_pos, DEFAULT_POS, -1));
  }
  if (building_options[5] && GetAllPositionsOfNeurons(SYNAPSE).size() < 2) {
     for (int i=0; i<8; i++) 
       actions_.push_back(AiOption(NUCLEUS, DEFAULT_POS, DEFAULT_POS, (float)i));
  }
  if (GetAllPositionsOfNeurons(SYNAPSE).size() > 0) {
    for (const auto& synapse_pos : GetAllPositionsOfNeurons(SYNAPSE)) {
      for (const auto& target_pos : enemies_.front()->GetAllPositionsOfNeurons()) {
        if (ran_gen_->RandomInt(0, 2) == 1)
          actions_.push_back(AiOption(200, synapse_pos, target_pos, -1));
      }
      for (const auto& target_pos : enemies_.front()->GetAllPositionsOfNeurons(ACTIVATEDNEURON)) {
        if (ran_gen_->RandomInt(0, 2) == 1)
          actions_.push_back(AiOption(250, synapse_pos, target_pos, -1));
      }
    }
  }
}
