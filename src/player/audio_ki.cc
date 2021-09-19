#include "player/audio_ki.h"
#include "audio/audio.h"
#include "constants/codes.h"
#include "game/field.h"
#include "objects/units.h"
#include "spdlog/spdlog.h"
#include "utils/utils.h"
#include <cstddef>
#include <iterator>
#include <shared_mutex>
#include <vector>

AudioKi::AudioKi(Position nucleus_pos, int iron, Player* player, Field* field, Audio* audio) 
    : Player(nucleus_pos, field, iron), 
    average_bpm_(audio->analysed_data().average_bpm_), average_level_(audio->analysed_data().average_level_) {
  player_ = player;
  audio_ = audio;
  max_activated_neurons_ = 3;
  nucleus_pos_ = nucleus_pos;
  intelligenz_ = 0;

  attack_strategies_ = {{Tactics::EPSP_FOCUSED, 1}, {Tactics::IPSP_FOCUSED, 1}, {Tactics::AIM_NUCLEUS, 1},
    {Tactics::DESTROY_ACTIVATED_NEURONS, 1}, {Tactics::DESTROY_SYNAPSES, 1}, 
    {Tactics::BLOCK_ACTIVATED_NEURON, 1}, {Tactics::BLOCK_SYNAPSES, 1}};
  defence_strategies_ = {{Tactics::DEF_FRONT_FOCUS, 1}, {Tactics::DEF_SURROUNG_FOCUS, 1}, 
    {Tactics::DEF_IPSP_BLOCK, 1}};
  technology_tactics_ = {{WAY, 1}, {SWARM, 1}, {TARGET, 1}, {TOTAL_OXYGEN, 1}, {TOTAL_RESOURCE, 1}, 
    {CURVE, 1}, {ATK_POTENIAL, 1}, {ATK_SPEED, 1}, {ATK_DURATION, 1}, {DEF_POTENTIAL, 1}, {DEF_SPEED, 1}, 
    {NUCLEUS_RANGE, 1}};
  resource_tactics_ = {{OXYGEN, 1}, {POTASSIUM, 1}, {CHLORIDE, 1}, {GLUTAMATE, 1}, {DOPAMINE, 1}, 
    {SEROTONIN, 1}};
  building_tactics_ = {{ACTIVATEDNEURON, 4}, {SYNAPSE, 1}, {NUCLEUS, 1}};
}

void AudioKi::SetUpTactics(Interval interval) {
  spdlog::get(LOGGER)->debug("AudioKi::SetUpTactics.");
  // Major: defence
  if (interval.major_) {
    resource_tactics_[GLUTAMATE] += 6;
    building_tactics_[ACTIVATEDNEURON] += 10;
    // Increase all defence-strategies
    for (auto& it : defence_strategies_)
      it.second += 5;
    // Additionally increase depending on signature.
    if (interval.signature_ == Signitue::SHARP)
      defence_strategies_[Tactics::DEF_FRONT_FOCUS] += 2;
    else if (interval.signature_ == Signitue::FLAT)
      defence_strategies_[Tactics::DEF_SURROUNG_FOCUS] += 2;
    else {
      defence_strategies_[Tactics::DEF_IPSP_BLOCK] += 2;
      resource_tactics_[CHLORIDE] += 2;  // needed to build ipsp.
      technology_tactics_[UnitsTech::TARGET] += 2;  // needed to target enemy epsp.
      technology_tactics_[UnitsTech::WAY] += 2;  // needed to target enemy epsp.
    }
  }
  // Minor: attack
  else {
    resource_tactics_[POTASSIUM] += 3;
    resource_tactics_[DOPAMINE] += 3;
    building_tactics_[SYNAPSE] += 5;
    // Increase all attack-strategies
    for (auto& it : attack_strategies_)
      it.second += 5;
    // Epsp/ ipsp depending on signature
    if (interval.signature_ == Signitue::UNSIGNED) {
      resource_tactics_[POTASSIUM] += 2;
      attack_strategies_[Tactics::EPSP_FOCUSED] += 2;
    }
    else {
      resource_tactics_[CHLORIDE] += 2;
      attack_strategies_[Tactics::IPSP_FOCUSED] += 2;
    }
    // targets/ brute force/ intelligent way depending on signature
    if (interval.signature_ == Signitue::FLAT) {
      technology_tactics_[UnitsTech::TARGET] += 5;
      if (interval.darkness_ > 4)
        attack_strategies_[DESTROY_ACTIVATED_NEURONS] += 2;
      else if (interval.darkness_ < 4) 
        attack_strategies_[DESTROY_ACTIVATED_NEURONS] += 2;
      if (interval.notes_out_key_ > 4)
        attack_strategies_[BLOCK_ACTIVATED_NEURON] += 2;
      else 
        attack_strategies_[BLOCK_SYNAPSES] += 2;
    }
    else if (interval.signature_ == Signitue::UNSIGNED) {
      technology_tactics_[UnitsTech::SWARM] += 5;
      attack_strategies_[AIM_NUCLEUS] += 3;
    }
    else if (interval.signature_ == Signitue::SHARP) {
      technology_tactics_[UnitsTech::WAY] += 5;
    }
  }

  // Sharp: refinement
  if (interval.signature_ == Signitue::SHARP) {
    resource_tactics_[POTASSIUM] += 5;
    resource_tactics_[DOPAMINE] += 5;
    if (interval.major_) {
      resource_tactics_[GLUTAMATE] += 2;
      technology_tactics_[DEF_POTENTIAL] += (interval.darkness_>4) ? 6 : 5;
      technology_tactics_[DEF_SPEED] += (interval.darkness_<4) ? 6 : 5;
    }
    else {
      resource_tactics_[POTASSIUM] += 2;
      technology_tactics_[ATK_POTENIAL] += (interval.darkness_>4) ? 6 : 5;
      technology_tactics_[ATK_POTENIAL] += (interval.darkness_>4) ? 6 : 5;
      technology_tactics_[ATK_DURATION] += (interval.notes_out_key_>5) ? 7 : 5;
    }
  }

  // Flat: Resources
  if (interval.signature_ == Signitue::FLAT) {
    resource_tactics_[POTASSIUM] += 5;
    resource_tactics_[DOPAMINE] += 5;
    technology_tactics_[TOTAL_OXYGEN] += (interval.darkness_>4) ? 6 : 5;
    technology_tactics_[TOTAL_RESOURCE] += (interval.darkness_>4) ? 6 : 5;
    technology_tactics_[CURVE] += (interval.notes_out_key_>5) ? 7 : 5;
  }

  // Minor + signed
  if (!interval.major_ && interval.signature_ != Signitue::UNSIGNED) {
    resource_tactics_[POTASSIUM] += 5;
    resource_tactics_[DOPAMINE] += 5;
    building_tactics_[NUCLEUS] += 10;
    technology_tactics_[NUCLEUS_RANGE] += 10;
  }

  attack_strategies_.at(interval.key_note_ % attack_strategies_.size())++;
  defence_strategies_.at(interval.key_note_ % defence_strategies_.size() + DEF_FRONT_FOCUS)++;
  technology_tactics_.at(interval.key_note_ % technology_tactics_.size() + WAY)++;
  resource_tactics_.at(interval.key_note_ % resource_tactics_.size() + OXYGEN)++;
  building_tactics_.at(interval.key_note_ % building_tactics_.size())++;
  
  // Log results:
  spdlog::get(LOGGER)->info("key {}, darkness {}, notes_out_key {}", interval.key_, interval.darkness_, 
      interval.notes_out_key_);
  for (const auto& it : attack_strategies_)
    spdlog::get(LOGGER)->info("{}: {}", tactics_mapping.at(it.first), it.second);
  for (const auto& it : defence_strategies_)
    spdlog::get(LOGGER)->info("{}: {}", tactics_mapping.at(it.first), it.second);
  for (const auto& it : technology_tactics_)
    spdlog::get(LOGGER)->info("{}: {}", units_tech_mapping.at(it.first), it.second);
  for (const auto& it : resource_tactics_)
    spdlog::get(LOGGER)->info("{}: {}", resources_name_mapping.at(it.first), it.second);
  for (const auto& it : building_tactics_)
    spdlog::get(LOGGER)->info("{}: {}", units_tech_mapping.at(it.first), it.second);
}

void AudioKi::DoAction(const AudioDataTimePoint& data_at_beat, bool level_switch) {
  spdlog::get(LOGGER)->debug("AudioKi::DoAction.");
  if (level_switch && data_at_beat.level_ > average_level_ && notes_played_.size() > 4) {
    CreateIpspThenEpsp(data_at_beat);
  }
  else if (level_switch && data_at_beat.level_ > average_level_) {
    CreateEpsp(data_at_beat);
    CreateSynapses();
  }
  if (level_switch && data_at_beat.level_ < average_level_) {
    // CreateActivatedNeuron();
  }
  // KeepOxygenLow(data_at_beat); 
  HandleIron(data_at_beat);
}

void AudioKi::CreateEpsp(const AudioDataTimePoint& data_at_beat) {
  spdlog::get(LOGGER)->debug("AudioKi::CreateEpsp.");
  // Build synapses if no synapses exists.
  if (GetAllPositionsOfNeurons(UnitsTech::SYNAPSE).size() < 1)
    return;
  // Get synapse-pos (TODO: changes randomnes), and update target (TODO: more flexible tagret-selection)
  auto synapse_pos = GetAllPositionsOfNeurons(UnitsTech::SYNAPSE).front();
  Position target_pos = player_->GetPositionOfClosestNeuron(synapse_pos, UnitsTech::NUCLEUS);
  if (notes_played_.size() > 4 && technologies_.at(UnitsTech::TARGET).second > 2)
    ChangeEpspTargetForSynapse(synapse_pos, target_pos);

  // Calculate update number of epsps to create and update interval
  double update_interval = 60000.0/(data_at_beat.bpm_*16);
  int num = data_at_beat.level_ - average_level_;
  auto last_epsp = std::chrono::steady_clock::now();  
  
  while (num > 0 && GetMissingResources(UnitsTech::EPSP).size() == 0) {
    // Add epsp in certain interval.
    auto cur_time = std::chrono::steady_clock::now(); 
    if (utils::get_elapsed(last_epsp, cur_time) < update_interval) 
      continue;

    // Add potential and update last-epsp.
    AddPotential(synapse_pos, UnitsTech::EPSP);
    last_epsp = std::chrono::steady_clock::now();
  }
}

void AudioKi::CreateIpsp(const AudioDataTimePoint& data_at_beat) {
  spdlog::get(LOGGER)->debug("AudioKi::CreateIpsp.");
  // Build synapses if no synapses exists.
  if (GetAllPositionsOfNeurons(UnitsTech::SYNAPSE).size() < 1)
    return;
  // Get synapse-pos (TODO: changes randomnes), and update target (TODO: more flexible tagret-selection)
  auto synapse_pos = GetAllPositionsOfNeurons(UnitsTech::SYNAPSE).front();
  Position target_pos = player_->GetPositionOfClosestNeuron(synapse_pos, UnitsTech::ACTIVATEDNEURON);
  if (notes_played_.size() > 6 && technologies_.at(UnitsTech::TARGET).second > 1)
    ChangeIpspTargetForSynapse(synapse_pos, target_pos);

  // Calculate update number of ipsps to create and update interval
  double update_interval = 60000.0/(data_at_beat.bpm_*16);
  int num = data_at_beat.level_ - average_level_;
  auto last_ipsp = std::chrono::steady_clock::now();  
  
  while (num > 0 && GetMissingResources(UnitsTech::EPSP).size() == 0) {
    // Add ipsp in certain interval.
    auto cur_time = std::chrono::steady_clock::now(); 
    if (utils::get_elapsed(last_ipsp, cur_time) < update_interval) 
      continue;

    // Add potential and update last-ipsp.
    AddPotential(synapse_pos, UnitsTech::EPSP);
    last_ipsp = std::chrono::steady_clock::now();
  }
}

void AudioKi::CreateIpspThenEpsp(const AudioDataTimePoint& data_at_beat) {
  spdlog::get(LOGGER)->debug("AudioKi::CreateIpspThenEpsp.");
  CreateIpsp(data_at_beat);
  auto start = std::chrono::steady_clock::now(); 
  auto cur_time = std::chrono::steady_clock::now(); 
  while(utils::get_elapsed(start, cur_time) < data_at_beat.bpm_)
    continue;
  CreateEpsp(data_at_beat);
}

void AudioKi::CreateSynapses(bool force) {
  spdlog::get(LOGGER)->debug("AudioKi::CreateIpspThenEpsp.");
  if (GetAllPositionsOfNeurons(UnitsTech::SYNAPSE).size() == 0 || force) {
    spdlog::get(LOGGER)->debug("AudioKi::CreateSynapses: creating synapses.");
    auto pos = field_->FindFree(nucleus_pos_.first, nucleus_pos_.second, 1, 5);
    if (AddNeuron(pos, UnitsTech::SYNAPSE, player_->nucleus_pos()))
      field_->AddNewUnitToPos(pos, UnitsTech::SYNAPSE);
    spdlog::get(LOGGER)->debug("AudioKi::CreateSynapses: done.");
  }
}

void AudioKi::CreateActivatedNeuron(bool force) {
  spdlog::get(LOGGER)->debug("AudioKi::CreateActivatedNeuron.");
  size_t num_activated_neurons = GetNumActivatedNeurons();
  if (num_activated_neurons >= max_activated_neurons_)
    return;
  // Only add def, if a barrak already exists, or no def exists.
  if (!(num_activated_neurons > 0 && GetAllPositionsOfNeurons(UnitsTech::SYNAPSE).size() == 0) || force) {
    spdlog::get(LOGGER)->debug("AudioKi::CreateActivatedNeuron: createing neuron");
    auto pos = field_->FindFree(nucleus_pos_.first, nucleus_pos_.second, 1, 5);
    if (AddNeuron(pos, UnitsTech::ACTIVATEDNEURON))
      field_->AddNewUnitToPos(pos, UnitsTech::ACTIVATEDNEURON);
    spdlog::get(LOGGER)->debug("AudioKi::CreateActivatedNeuron: done");
  }
}

void AudioKi::HandleIron(const AudioDataTimePoint& data_at_beat) {
  spdlog::get(LOGGER)->debug("AudioKi::HandleIron.");

  sorted_stragety sorted_resources = SortStrategy(resource_tactics_);
  for (const auto& it : sorted_resources) {
    size_t resource = it.second;
    std::string resource_name = "unknown";
    if (resources_name_mapping.count(resource) > 0) 
      resources_name_mapping.at(resource);
    spdlog::get(LOGGER)->debug("AudioKi::HandleIron: checking {}: {}.", resource, resource_name);
    if (resource == OXYGEN || !IsActivatedResource(resource)) {
      spdlog::get(LOGGER)->debug("AudioKi::HandleIron: distributing: {} {}.", resource, resource_name);
      DistributeIron(resource);
    }
  }
  spdlog::get(LOGGER)->debug("AudioKi::HandleIron: done.");
}

void AudioKi::NewTechnology(const AudioDataTimePoint& data_at_beat) {
  spdlog::get(LOGGER)->debug("AudioKi::NewTechnology.");
  std::list<std::pair<size_t, size_t>> sorted_resources;
  for (const auto& it : technology_tactics_)
    sorted_resources.push_back({it.second, it.first});
  sorted_resources.sort();
  sorted_resources.reverse();

  for (const auto& it : sorted_resources) {
    if (technologies_.at(it.second) < technologies_.at(it.second)) {
      AddTechnology(it.second);
      break;
    }
  }
}

void AudioKi::KeepOxygenLow(const AudioDataTimePoint& data_at_beat) {
  std::shared_lock sl(mutex_resources_);
  // Check if keeping oxygen low is acctually needed.
  bool all_activated = true;
  for (const auto& it : resources_)
    if (!it.second.second) all_activated = false;
  if (all_activated || resources_.at(Resources::OXYGEN).first < 10) 
    return;
  sl.unlock();

  // Check if investing oxygen now, will leed to oxygen beeing low enough, when
  // next iron boast comes up.
  if (audio_->NextOfNotesIn(data_at_beat.time_) > 5)
    return;
  auto potassium_activated = resources_.at(Resources::POTASSIUM).second;
  if (attack_focus_ && potassium_activated)
    CreateSynapses(true);
  else 
    CreateActivatedNeuron(true);
}

AudioKi::sorted_stragety AudioKi::SortStrategy(std::map<int, int> strategy) {
  std::list<std::pair<size_t, size_t>> sorted_strategy;
  for (const auto& it : strategy)
    sorted_strategy.push_back({it.second, it.first});
  sorted_strategy.sort();
  sorted_strategy.reverse();
  return sorted_strategy;
}
