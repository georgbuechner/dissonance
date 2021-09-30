#include "player/audio_ki.h" 
#include "audio/audio.h"
#include "constants/codes.h"
#include "constants/costs.h"
#include "game/field.h"
#include "objects/units.h"
#include "spdlog/spdlog.h"
#include "utils/utils.h"
#include <cstddef>
#include <iterator>
#include <mutex>
#include <algorithm>
#include <shared_mutex>
#include <vector>

AudioKi::AudioKi(Position nucleus_pos, int iron, Field* field, Audio* audio) 
    : Player(nucleus_pos, field, iron, audio), 
    average_bpm_(audio->analysed_data().average_bpm_), average_level_(audio->analysed_data().average_level_) {
  max_activated_neurons_ = 3;
  nucleus_pos_ = nucleus_pos;
  intelligenz_ = 0;
  cur_interval_ = audio_->analysed_data().intervals_[0];

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

  for (int i=0; i<nucleus_.max_voltage(); i++) {
    if (i > 7)
      extra_nucleus_[i] = 2;
    else if (i>4)
      extra_nucleus_[i] = 1;
    else 
      extra_nucleus_[i] = 0;
  }
}

void AudioKi::set_last_time_point(const AudioDataTimePoint &data_at_beat) {
  last_data_point_ = data_at_beat;
}

void AudioKi::SetUpTactics() {
  spdlog::get(LOGGER)->debug("AudioKi::SetUpTactics.");
  // Major: defence
  if (cur_interval_.major_) {
    resource_tactics_[GLUTAMATE] += 6;
    building_tactics_[ACTIVATEDNEURON] += 10;
    // Increase all defence-strategies
    for (auto& it : defence_strategies_)
      it.second += 5;
    // Additionally increase depending on signature.
    if (cur_interval_.signature_ == Signitue::SHARP)
      defence_strategies_[Tactics::DEF_FRONT_FOCUS] += 2;
    else if (cur_interval_.signature_ == Signitue::FLAT)
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
    if (cur_interval_.signature_ == Signitue::UNSIGNED) {
      resource_tactics_[POTASSIUM] += 2;
      attack_strategies_[Tactics::EPSP_FOCUSED] += 2;
    }
    else {
      resource_tactics_[CHLORIDE] += 2;
      technology_tactics_[UnitsTech::TARGET] += 5;
      attack_strategies_[Tactics::IPSP_FOCUSED] += 2;
    }
    // targets/ brute force/ intelligent way depending on signature
    if (cur_interval_.signature_ == Signitue::FLAT) {
      technology_tactics_[UnitsTech::TARGET] += 5;
      if (cur_interval_.darkness_ > 4)
        attack_strategies_[DESTROY_ACTIVATED_NEURONS] += 2;
      else if (cur_interval_.darkness_ < 4) 
        attack_strategies_[DESTROY_ACTIVATED_NEURONS] += 2;
      if (cur_interval_.notes_out_key_ > 4)
        attack_strategies_[BLOCK_ACTIVATED_NEURON] += 2;
      else 
        attack_strategies_[BLOCK_SYNAPSES] += 2;
    }
    else if (cur_interval_.signature_ == Signitue::UNSIGNED) {
      technology_tactics_[UnitsTech::SWARM] += 5;
      attack_strategies_[AIM_NUCLEUS] += 3;
    }
    else if (cur_interval_.signature_ == Signitue::SHARP) {
      technology_tactics_[UnitsTech::WAY] += 5;
    }
  }

  // Sharp: refinement
  if (cur_interval_.signature_ == Signitue::SHARP) {
    resource_tactics_[SEROTONIN] += 5;
    resource_tactics_[DOPAMINE] += 5;
    if (cur_interval_.major_) {
      resource_tactics_[GLUTAMATE] += 2;
      technology_tactics_[DEF_POTENTIAL] += (cur_interval_.darkness_>4) ? 6 : 5;
      technology_tactics_[DEF_SPEED] += (cur_interval_.darkness_<4) ? 6 : 5;
    }
    else {
      resource_tactics_[POTASSIUM] += 2;
      technology_tactics_[ATK_POTENIAL] += (cur_interval_.darkness_>4) ? 6 : 5;
      technology_tactics_[ATK_POTENIAL] += (cur_interval_.darkness_>4) ? 6 : 5;
      technology_tactics_[ATK_DURATION] += (cur_interval_.notes_out_key_>5) ? 7 : 5;
    }
  }

  // Flat: Resources
  if (cur_interval_.signature_ == Signitue::FLAT) {
    resource_tactics_[POTASSIUM] += 5;
    resource_tactics_[SEROTONIN] += 6;
    resource_tactics_[DOPAMINE] += 6;
    technology_tactics_[TOTAL_OXYGEN] += (cur_interval_.darkness_>4) ? 6 : 5;
    technology_tactics_[TOTAL_RESOURCE] += (cur_interval_.darkness_>4) ? 6 : 5;
    technology_tactics_[CURVE] += (cur_interval_.notes_out_key_>5) ? 7 : 5;
  }

  // Minor + signed
  if (!cur_interval_.major_ && cur_interval_.signature_ != Signitue::UNSIGNED) {
    resource_tactics_[POTASSIUM] += 5;
    resource_tactics_[SEROTONIN] += 8;
    resource_tactics_[DOPAMINE] += 7;
    building_tactics_[NUCLEUS] += 10;
    technology_tactics_[NUCLEUS_RANGE] += 10;
  }

  // Add more random factor: key_note
  attack_strategies_.at(cur_interval_.key_note_ % attack_strategies_.size())++;
  defence_strategies_.at(cur_interval_.key_note_ % defence_strategies_.size() + DEF_FRONT_FOCUS)++;
  technology_tactics_.at(cur_interval_.key_note_ % technology_tactics_.size() + WAY)++;
  resource_tactics_.at(cur_interval_.key_note_ % resource_tactics_.size() + OXYGEN)++;
  building_tactics_.at(cur_interval_.key_note_ % building_tactics_.size())++;
  // Add more random factor: darkness
  attack_strategies_.at(cur_interval_.darkness_ % attack_strategies_.size())++;
  defence_strategies_.at(cur_interval_.darkness_ % defence_strategies_.size() + DEF_FRONT_FOCUS)++;
  technology_tactics_.at(cur_interval_.darkness_% technology_tactics_.size() + WAY)++;
  resource_tactics_.at(cur_interval_.darkness_% resource_tactics_.size() + OXYGEN)++;
  building_tactics_.at(cur_interval_.darkness_ % building_tactics_.size())++;
  // Add more random factor: notes outside of key
  attack_strategies_.at(cur_interval_.notes_out_key_ % attack_strategies_.size())++;
  defence_strategies_.at(cur_interval_.notes_out_key_ % defence_strategies_.size() + DEF_FRONT_FOCUS)++;
  technology_tactics_.at(cur_interval_.notes_out_key_ % technology_tactics_.size() + WAY)++;
  resource_tactics_.at(cur_interval_.notes_out_key_ % resource_tactics_.size() + OXYGEN)++;
  building_tactics_.at(cur_interval_.notes_out_key_ % building_tactics_.size())++;

  balancing_ = cur_interval_.darkness_ % 2;

  // Log results:
  spdlog::get(LOGGER)->info("key {}, darkness {}, notes_out_key {}", cur_interval_.key_, cur_interval_.darkness_, 
      cur_interval_.notes_out_key_);
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

  int aim_nuclus = attack_strategies_.at(AIM_NUCLEUS);
  int destroy_activated_neurons = attack_strategies_.at(DESTROY_ACTIVATED_NEURONS);
  int destroy_synapses = attack_strategies_.at(DESTROY_SYNAPSES);
  epsp_target_strategy_ = AIM_NUCLEUS;  // default.
  if (destroy_activated_neurons > aim_nuclus && destroy_activated_neurons > destroy_synapses)
    epsp_target_strategy_ = DESTROY_ACTIVATED_NEURONS;
  else if (destroy_synapses > aim_nuclus && destroy_synapses > destroy_activated_neurons)
    epsp_target_strategy_ = DESTROY_SYNAPSES;
  ipsp_target_strategy_ = BLOCK_ACTIVATED_NEURON; // default.
  if (attack_strategies_.at(BLOCK_SYNAPSES) > attack_strategies_.at(BLOCK_ACTIVATED_NEURON))
    ipsp_target_strategy_ = BLOCK_SYNAPSES;

  if (cur_interval_.id_+1 < audio_->analysed_data().intervals_.size())
    cur_interval_ = audio_->analysed_data().intervals_[cur_interval_.id_+1];
}

void AudioKi::DoAction(const AudioDataTimePoint& data_at_beat) {
  spdlog::get(LOGGER)->debug("AudioKi::DoAction.");
  // Change tactics when interval changes:
  if (data_at_beat.interval_ > last_data_point_.interval_)
    SetUpTactics();
  // Create synapses when switch above average level.
  if (data_at_beat.level_ > average_level_ && last_data_points_above_average_level_.size() == 0) 
    CreateSynapses();
  // Add data_at_beat to list of beats since switching above average.
  if (data_at_beat.level_ > average_level_) 
    last_data_points_above_average_level_.push_back(data_at_beat);
  // Launch attack, when above average wave is over.
  if (data_at_beat.level_ <= average_level_ && last_data_points_above_average_level_.size() > 0) {
    LaunchAttack(data_at_beat);
    last_data_points_above_average_level_.clear();
  }
  // Create activated neuron if level drops below average.
  if (last_data_point_.level_ >= average_level_ && data_at_beat.level_ < average_level_) {
    CreateActivatedNeuron();
  }
  if (extra_nucleus_.at(nucleus_.voltage()) > 0) {
    CreateActivatedNeuron(true);
    extra_nucleus_.at(nucleus_.voltage())--;
  }
  if (audio_->MoreOfNotes(data_at_beat, false))
    NewTechnology(data_at_beat);
  // KeepOxygenLow(data_at_beat); 
  HandleIron(data_at_beat);
}

void AudioKi::LaunchAttack(const AudioDataTimePoint& data_at_beat) {
  spdlog::get(LOGGER)->debug("AudioKi::LaunchAttack.");
  // Sort synapses (use synapses futhest from enemy for epsp)
  auto sorted_synapses = SortPositionsByDistance(enemy_->nucleus_pos(), GetAllPositionsOfNeurons(UnitsTech::SYNAPSE));
  if (sorted_synapses.size() == 0)
    return;
  std::unique_lock ul(mutex_all_neurons_);
  auto epsp_synapses = neurons_.at(sorted_synapses.back());
  auto epsp_way = field_->GetWayForSoldier(epsp_synapses->pos_, epsp_synapses->GetWayPoints(UnitsTech::EPSP));
  ul.unlock();
 
  spdlog::get(LOGGER)->debug("AudioKi::LaunchAttack: Get ipsp targets");
  auto ipsp_launch_synapes = AvailibleIpspLaunches(sorted_synapses, 5);
  auto ipsp_targets = GetIpspTargets(epsp_way, sorted_synapses);  // using epsp-way, since we want to clear this way.
  spdlog::get(LOGGER)->debug("AudioKi::LaunchAttack: Got {} ipsp targets.", ipsp_targets.size());
  spdlog::get(LOGGER)->debug("AudioKi::LaunchAttack: Get epsp target");
  Position epsp_target;
  // Take first target which is not already ipsp target.
  for (const auto& it : GetEpspTargets(sorted_synapses.back(), epsp_way))
    if (std::find(ipsp_targets.begin(), ipsp_targets.end(), it) == ipsp_targets.end())
      epsp_target = it;

  // Check whether to launch attack.
  size_t available_ipsps = AvailibleIpsps();
  size_t num_epsps_to_create = GetLaunchAttack(data_at_beat, available_ipsps);
  spdlog::get(LOGGER)->debug("AudioKi::LaunchAttack: targeting to create epsps {}.", num_epsps_to_create);

  // Launch ipsp attacks first. Only launch if epsp attack is launched too or
  // strategy is blocking enemy synapses.
  if (ipsp_target_strategy_ == BLOCK_SYNAPSES || num_epsps_to_create > 0) {
    spdlog::get(LOGGER)->debug("AudioKi::LaunchAttack: launching ipsp attacks...");
    size_t available_ipsps = AvailibleIpsps();
    for (size_t i=0; i<ipsp_targets.size() && i<ipsp_launch_synapes.size(); i++) {
      CreateIpsps(ipsp_launch_synapes[i], ipsp_targets[i], available_ipsps/ipsp_targets.size(), data_at_beat.bpm_);
    }
  }
  // Only launch if expected number of epsps to create is reached.
  if (num_epsps_to_create > 0) {
    if (ipsp_launch_synapes.size() > 0) {
      ul.lock();
      auto ipsp_way = field_->GetWayForSoldier(ipsp_launch_synapes.front(), 
          neurons_.at(ipsp_launch_synapes.front())->GetWayPoints(UnitsTech::IPSP));
      ul.unlock();
      SynchAttacks(epsp_way.size(), ipsp_way.size());
    }
    spdlog::get(LOGGER)->debug("AudioKi::LaunchAttack: launching epsp attack...");
    // Launch epsps next.
    CreateEpsps(sorted_synapses.back(), epsp_target, data_at_beat.bpm_);
  }

  // Reset target for all synapses
  spdlog::get(LOGGER)->debug("AudioKi::LaunchAttack: reseting target-positions.");
  for (const auto& it : sorted_synapses) {
    ChangeEpspTargetForSynapse(it, enemy_->nucleus_pos());
    ChangeIpspTargetForSynapse(it, enemy_->nucleus_pos());
  }
}

std::vector<Position> AudioKi::GetEpspTargets(Position synapse_pos, std::list<Position> way, size_t ignore_strategy) {
  spdlog::get(LOGGER)->debug("AudioKi::GetEpspTargets");
  if (technologies_.at(UnitsTech::TARGET).first < 2)  {
    spdlog::get(LOGGER)->debug("AudioKi::GetEpspTargets: using default.");
    return {enemy_->nucleus_pos()};
  }
  if (epsp_target_strategy_ == DESTROY_ACTIVATED_NEURONS && ignore_strategy != DESTROY_ACTIVATED_NEURONS) {
    spdlog::get(LOGGER)->debug("AudioKi::GetEpspTargets: using DESTROY_ACTIVATED_NEURONS.");
    auto activated_neurons_on_way = GetAllActivatedNeuronsOnWay(way);
    activated_neurons_on_way = SortPositionsByDistance(nucleus_pos_, activated_neurons_on_way, false);
    return activated_neurons_on_way;
  }
  else if (epsp_target_strategy_ == DESTROY_SYNAPSES && ignore_strategy != DESTROY_SYNAPSES) {
    spdlog::get(LOGGER)->debug("AudioKi::GetEpspTargets: using DESTROY_SYNAPSES.");
    auto enemy_synapses = GetEnemySynapsesSortedByLeastDef(synapse_pos);
    if (enemy_synapses.size() == 0)
      return GetEpspTargets(synapse_pos, way, DESTROY_SYNAPSES);
    return enemy_synapses;
  }
  else {
   spdlog::get(LOGGER)->debug("AudioKi::GetEpspTargets: using AIM_NUCLEUS.");
   return {enemy_->GetPositionOfClosestNeuron(synapse_pos, UnitsTech::NUCLEUS)};
  }
}

std::vector<Position> AudioKi::GetIpspTargets(std::list<Position> way, std::vector<Position>& synapses, size_t ignore_strategy) {
  spdlog::get(LOGGER)->debug("AudioKi::GetIpspTargets");
  if (technologies_.at(UnitsTech::TARGET).first == 0) {
  spdlog::get(LOGGER)->debug("AudioKi::GetIpspTargets: using default.");
    return {enemy_->GetRandomNeuron()};
  }
  std::vector<Position> isps_targets;
  if (ipsp_target_strategy_ == BLOCK_ACTIVATED_NEURON && ignore_strategy != BLOCK_ACTIVATED_NEURON) {
  spdlog::get(LOGGER)->debug("AudioKi::GetIpspTargets: using BLOCK_ACTIVATED_NEURON.");
    auto activated_neurons_on_way = GetAllActivatedNeuronsOnWay(way);
    activated_neurons_on_way = SortPositionsByDistance(nucleus_pos_, activated_neurons_on_way, false);
    for (size_t i=0; i<synapses.size(); i++)
      isps_targets.push_back(activated_neurons_on_way[i]);
  }
  else if (epsp_target_strategy_ == BLOCK_SYNAPSES && ignore_strategy != BLOCK_SYNAPSES) {
  spdlog::get(LOGGER)->debug("AudioKi::GetIpspTargets: using BLOCK_SYNAPSES.");
    auto enemy_synapses = GetEnemySynapsesSortedByLeastDef(nucleus_pos_);
    if (enemy_synapses.size() == 0)
      return GetIpspTargets(way, synapses, BLOCK_SYNAPSES);
    for (size_t i=0; i<synapses.size(); i++)
      isps_targets.push_back(enemy_synapses[i]);
  }
  return isps_targets;
}

void AudioKi::CreateEpsps(Position synapse_pos, Position target_pos, int bpm) {
  spdlog::get(LOGGER)->debug("AudioKi::CreateEpsp.");
  ChangeEpspTargetForSynapse(synapse_pos, target_pos);
  // Calculate update number of epsps to create and update interval
  double update_interval = 60000.0/(bpm*16);
  auto last_epsp = std::chrono::steady_clock::now();  
  while (GetMissingResources(UnitsTech::EPSP).size() == 0) {
    // Add epsp in certain interval.
    if (utils::get_elapsed(last_epsp, std::chrono::steady_clock::now()) < update_interval) {
      AddPotential(synapse_pos, UnitsTech::EPSP);
      last_epsp = std::chrono::steady_clock::now();
    }
  }
}

void AudioKi::CreateIpsps(Position synapse_pos, Position target_pos, int num_ipsp_to_create, int bpm) {
  spdlog::get(LOGGER)->debug("AudioKi::CreateIpsp.");
  ChangeIpspTargetForSynapse(synapse_pos, target_pos);

  // Calculate update number of ipsps to create and update interval
  double update_interval = 60000.0/(bpm*16);
  auto last_ipsp = std::chrono::steady_clock::now();  
  while (GetMissingResources(UnitsTech::IPSP).size() == 0) {
    // Add ipsp in certain interval.
    if (utils::get_elapsed(last_ipsp, std::chrono::steady_clock::now()) >= update_interval) {
      AddPotential(synapse_pos, UnitsTech::IPSP);
      last_ipsp = std::chrono::steady_clock::now();
    }
  }
}

void AudioKi::CreateSynapses(bool force) {
  spdlog::get(LOGGER)->debug("AudioKi::Synapse.");
  int availible_oxygen = max_oxygen_ - bound_oxygen_;
  if (GetAllPositionsOfNeurons(UnitsTech::SYNAPSE).size() <= building_tactics_[SYNAPSE] && availible_oxygen > 25) {
    spdlog::get(LOGGER)->debug("AudioKi::CreateSynapses: creating synapses.");
    auto pos = field_->FindFree(nucleus_pos_.first, nucleus_pos_.second, 1, 5);
    if (AddNeuron(pos, UnitsTech::SYNAPSE, enemy_->nucleus_pos()))
      field_->AddNewUnitToPos(pos, UnitsTech::SYNAPSE);
    spdlog::get(LOGGER)->debug("AudioKi::CreateSynapses: done.");
  }
}

void AudioKi::CreateActivatedNeuron(bool force) {
  spdlog::get(LOGGER)->debug("AudioKi::CreateActivatedNeuron.");
  size_t num_activated_neurons = GetAllPositionsOfNeurons(ACTIVATEDNEURON).size();
  int availible_oxygen = max_oxygen_ - bound_oxygen_;
  if (num_activated_neurons >= building_tactics_[ACTIVATEDNEURON] || availible_oxygen < 25)
    return;
  // Only add def, if a synapses already exists, or no def exists.
  if (!(num_activated_neurons > 0 && GetAllPositionsOfNeurons(UnitsTech::SYNAPSE).size() == 0)) {
    spdlog::get(LOGGER)->debug("AudioKi::CreateActivatedNeuron: createing neuron");

    // Find position to place neuron coresponding to tactics.
    Position pos;
    if (SortStrategy(defence_strategies_).front().second == DEF_SURROUNG_FOCUS) {
      for (int i=1; i<cur_range_; i++) {
        auto positions = field_->GetAllInRange(nucleus_pos_, i, i-1, true);
        if (positions.size() > 0)
          pos = positions.front();
      }
    }
    else {
      auto way = field_->GetWayForSoldier(nucleus_pos_, {enemy_->nucleus_pos()});
      auto positions = field_->GetAllInRange(nucleus_pos_, cur_range_, 1, true);
      int max_way_points_in_range = 0;
      pos = positions.front();
      for (const auto& position : positions) {
        int counter = 0;
        int way_points_in_range = 0;
        for (const auto& way_point : way) {
          if (counter++ > cur_range_+3) 
            break;
          if (utils::dist(position, way_point) <= 3)
            way_points_in_range++;
        }
        if (way_points_in_range > max_way_points_in_range) {
          max_way_points_in_range = way_points_in_range;
          pos = position;
        }
      }
    }
    if (AddNeuron(pos, UnitsTech::ACTIVATEDNEURON)) {
      field_->AddNewUnitToPos(pos, UnitsTech::ACTIVATEDNEURON);
      spdlog::get(LOGGER)->debug("AudioKi::CreateActivatedNeuron: created activated neuron.");
    }
    else
      spdlog::get(LOGGER)->debug("AudioKi::CreateActivatedNeuron: not enough resources");
  }
}

void AudioKi::HandleIron(const AudioDataTimePoint& data_at_beat) {
  spdlog::get(LOGGER)->debug("AudioKi::HandleIron.");

  sorted_stragety sorted_resources = SortStrategy(resource_tactics_);
  for (const auto& it : sorted_resources) {
    size_t resource = it.second;
    std::string resource_name = "unknown";
    if (resources_name_mapping.count(resource) > 0) 
      resource_name = resources_name_mapping.at(resource);
    spdlog::get(LOGGER)->debug("AudioKi::HandleIron: checking {}: {}.", resource, resource_name);
    if (resource == OXYGEN && iron() > 2) {
      spdlog::get(LOGGER)->debug("AudioKi::HandleIron: distributing: {} {}.", resource, resource_name);
      DistributeIron(resource);
    }
    else if (resource != OXYGEN && !IsActivatedResource(resource)) {
      spdlog::get(LOGGER)->debug("AudioKi::HandleIron: distributing: {} {}.", resource, resource_name);
      DistributeIron(resource);
      break;
    }
  }
  spdlog::get(LOGGER)->debug("AudioKi::HandleIron: done.");
}

void AudioKi::NewTechnology(const AudioDataTimePoint& data_at_beat) {
  spdlog::get(LOGGER)->debug("AudioKi::NewTechnology.");
  std::list<std::pair<size_t, size_t>> sorted_technology_strategies = SortStrategy(technology_tactics_);
  std::shared_lock sl(mutex_technologies_);
  
  // Get least resourced technology.
  size_t max=0;
  for (const auto& it : technologies_) {
    size_t cur = it.second.second - it.second.first;
    if (max < cur) 
      max = cur;
  }

  for (const auto& it : sorted_technology_strategies) {
    const auto& cur_technology = technologies_.at(it.second);
    if (cur_technology.first < cur_technology.second && cur_technology.second-cur_technology.first < max+balancing_) {
      sl.unlock();
      bool success = AddTechnology(it.second);
      spdlog::get(LOGGER)->debug("AudioKi::NewTechnology: {} success: {}", units_tech_mapping.at(it.second), success);
      return;
    }
  }
}

void AudioKi::KeepOxygenLow(const AudioDataTimePoint& data_at_beat) {
  spdlog::get(LOGGER)->debug("AudioKi::KeepOxygenLow.");
  std::shared_lock sl(mutex_resources_);
  // Check if keeping oxygen low is acctually needed.
  bool all_activated = true;
  for (const auto& it : resources_)
    if (!it.second.second) 
      all_activated = false;
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

AudioKi::sorted_stragety AudioKi::SortStrategy(std::map<size_t, size_t> strategy) {
  std::list<std::pair<size_t, size_t>> sorted_strategy;
  for (const auto& it : strategy)
    sorted_strategy.push_back({it.second, it.first});
  sorted_strategy.sort();
  sorted_strategy.reverse();
  return sorted_strategy;
}


size_t AudioKi::AvailibleIpsps() {
  spdlog::get(LOGGER)->debug("AudioKi::AvailibleIpsps.");
  std::shared_lock sl(mutex_resources_);
  auto costs = units_costs_.at(UnitsTech::IPSP);
  size_t res = std::min(resources_.at(POTASSIUM).first / costs[POTASSIUM], resources_.at(CHLORIDE).first / costs[CHLORIDE]);
  spdlog::get(LOGGER)->debug("AudioKi::AvailibleIpsps: available ipsps: {}", res);
  if (attack_strategies_.at(EPSP_FOCUSED) > attack_strategies_.at(IPSP_FOCUSED)) {
    res *= attack_strategies_.at(IPSP_FOCUSED)/attack_strategies_.at(EPSP_FOCUSED);
    spdlog::get(LOGGER)->debug("AudioKi::AvailibleIpsps: available ipsps taking ipsp-/ epsp-focus under account: {}", res);
  }
  return res;
}

size_t AudioKi::AvailibleEpsps(size_t ipsps_to_create) {
  spdlog::get(LOGGER)->debug("AudioKi::AvailibleEpsps.");
  std::shared_lock sl(mutex_resources_);
  auto costs_ipsp = units_costs_.at(UnitsTech::IPSP);
  auto costs = units_costs_.at(UnitsTech::EPSP);
  size_t res = resources_.at(POTASSIUM).first / (costs[POTASSIUM] + ipsps_to_create*costs_ipsp[POTASSIUM]);
  return res;
}


std::vector<Position> AudioKi::AvailibleIpspLaunches(std::vector<Position>& synapses, int min) {
  spdlog::get(LOGGER)->debug("AudioKi::AvailibleIpspLaunches.");
  size_t available_ipsps = AvailibleIpsps();  
  std::vector<Position> result_positions;
  for (size_t i=0; i<available_ipsps; i+=min)
    result_positions.push_back(synapses[i]);
  spdlog::get(LOGGER)->debug("AudioKi::AvailibleIpspLaunches: got {} launch positions", result_positions.size());
  return result_positions;
}

std::vector<Position> AudioKi::GetAllActivatedNeuronsOnWay(std::list<Position> way) {
  spdlog::get(LOGGER)->debug("AudioKi::GetAllActivatedNeuronsOnWay.");
  auto enemy_activated_neurons = enemy_->GetAllPositionsOfNeurons(UnitsTech::ACTIVATEDNEURON);
  std::vector<Position> result_positions;
  for (const auto& way_point : way) {
    for (const auto& activated_neuron_pos : enemy_activated_neurons) {
      if (utils::dist(way_point, activated_neuron_pos) < 3)
        result_positions.push_back(activated_neuron_pos);
    }
  }
  spdlog::get(LOGGER)->debug("AudioKi::GetAllActivatedNeuronsOnWay: got {} activated neurons on way.", result_positions.size());
  return result_positions;
}

std::vector<Position> AudioKi::SortPositionsByDistance(Position start, std::vector<Position> positions, bool reverse) {
  spdlog::get(LOGGER)->debug("AudioKi::SortPositionsByDistance.");
  std::list<std::pair<size_t, Position>> sorted_positions;
  for (const auto& it : positions)
    sorted_positions.push_back({utils::dist(start, it), it});
  sorted_positions.sort();
  if (reverse)
    sorted_positions.reverse();
  std::vector<Position> result_positions;
  for (const auto& it : sorted_positions)
    result_positions.push_back(it.second);
  spdlog::get(LOGGER)->debug("AudioKi::SortPositionsByDistance: done.");
  return result_positions; 
}

std::vector<Position> AudioKi::GetEnemySynapsesSortedByLeastDef(Position start) {
  spdlog::get(LOGGER)->debug("AudioKi::GetEnemySynapsesSortedByLeastDef.");
  auto enemy_synapses = enemy_->GetAllPositionsOfNeurons(UnitsTech::SYNAPSE);
  std::list<std::pair<size_t, Position>> sorted_positions;
  for (const auto& it : enemy_synapses) {
    auto way = field_->GetWayForSoldier(start, {it});
    sorted_positions.push_back({GetAllActivatedNeuronsOnWay(way).size(), it});
  }
  sorted_positions.sort();
  sorted_positions.reverse();
  std::vector<Position> result_positions;
  for (const auto& it : sorted_positions)
    result_positions.push_back(it.second);
  return result_positions; 
}

size_t AudioKi::GetMaxLevelExeedance() const {
  size_t cur_max = 0;
  for (const auto& it : last_data_points_above_average_level_) {
    size_t diff = it.level_ - average_level_;
    if (diff > cur_max)
      cur_max = diff;
  }
  return cur_max;
}

size_t AudioKi::GetLaunchAttack(const AudioDataTimePoint& data_at_beat, size_t ipsps_to_create) {
  size_t num_epsps_to_create = GetMaxLevelExeedance();
  size_t available_epsps = AvailibleEpsps(ipsps_to_create);
  // Always launch attack in first interval.
  if (cur_interval_.id_ == 0)
    return num_epsps_to_create;
  // From 4th interval onwards, add darkness as factor for number of epsps to create.
  if (cur_interval_.id_ > 3) {
    num_epsps_to_create *= cur_interval_.darkness_*0.125*cur_interval_.id_;
    spdlog::get(LOGGER)->info("AudioKi::GetLaunchAttack: darkness: {}, epsps to create: {}, available: {}", 
        cur_interval_.darkness_, num_epsps_to_create, available_epsps);
  }

  // Crop, to not exeed a to high number of potassium:
  auto costs_epsp = units_costs_.at(UnitsTech::EPSP);
  double diff = max_resources_ * 0.6 - num_epsps_to_create * costs_epsp[POTASSIUM]; // max-to-spend - total costs.
  if (diff < 0)
    num_epsps_to_create += diff/costs_epsp[POTASSIUM]; // + because diff is negative
  spdlog::get(LOGGER)->info("AudioKi::GetLaunchAttack: epsps to create: {}, available: {}", 
      num_epsps_to_create, available_epsps);
  // Now, only launch, if target-amount can be reached.
  return (num_epsps_to_create > available_epsps) ? 0 : num_epsps_to_create;
}

void AudioKi::SynchAttacks(size_t epsp_way_length, size_t ipsp_way_length) {
  std::shared_lock sl_technologies(mutex_technologies_);
  int speed_boast = 50*technologies_.at(UnitsTech::ATK_POTENIAL).first;
  sl_technologies.unlock();
  size_t ipsp_duration = ipsp_way_length*(420-speed_boast);
  size_t epsp_duration = epsp_way_length*(370-speed_boast);
  int wait_time = (ipsp_duration-epsp_duration) + 100;
  spdlog::get(LOGGER)->info("AudioKi::SynchAttacks: Waiting for {} millisecond.", wait_time);
  auto start = std::chrono::steady_clock::now();  
  while (utils::get_elapsed(start, std::chrono::steady_clock::now()) < wait_time) { continue; }
  spdlog::get(LOGGER)->info("AudioKi::SynchAttacks: Done waiting.");
}
