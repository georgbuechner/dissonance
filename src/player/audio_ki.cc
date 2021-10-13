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

AudioKi::AudioKi(position_t nucleus_pos, Field* field, Audio* audio, RandomGenerator* ran_gen,
    std::map<int, position_t> resource_positions) 
  : Player(nucleus_pos, field, ran_gen, resource_positions), 
    average_bpm_(audio->analysed_data().average_bpm_), 
    average_level_(audio->analysed_data().average_level_) 
{
  audio_ = audio;
  max_activated_neurons_ = 3;
  nucleus_pos_ = nucleus_pos;
  cur_interval_ = audio_->analysed_data().intervals_[0];

  // TODO (fux): increase iron by one.
  attack_strategies_ = {{Tactics::EPSP_FOCUSED, 1}, {Tactics::IPSP_FOCUSED, 1}, {Tactics::AIM_NUCLEUS, 1},
    {Tactics::DESTROY_ACTIVATED_NEURONS, 1}, {Tactics::DESTROY_SYNAPSES, 1}, 
    {Tactics::BLOCK_ACTIVATED_NEURON, 1}, {Tactics::BLOCK_SYNAPSES, 1}};
  defence_strategies_ = {{Tactics::DEF_FRONT_FOCUS, 1}, {Tactics::DEF_SURROUNG_FOCUS, 1}, 
    {Tactics::DEF_IPSP_BLOCK, 1}};
  building_tactics_ = {{ACTIVATEDNEURON, 4}, {SYNAPSE, 1}, {NUCLEUS, 1}};

  // Set additional activated neurons for increased voltage in nucleus.
  for (int i=0; i<nucleus_.max_voltage(); i++) {
    if (i > 7) extra_activated_neurons_[i] = 2;
    else if (i>4) extra_activated_neurons_[i] = 1;
    else extra_activated_neurons_[i] = 0;
  }
}

void AudioKi::set_last_time_point(const AudioDataTimePoint &data_at_beat) {
  last_data_point_ = data_at_beat;
}

void AudioKi::SetUpTactics(bool economy_tactics) {
  // Setup tactics.
  SetBattleTactics();
  if (economy_tactics)
    SetEconomyTactics();
  
  // Increase interval.
  if (cur_interval_.id_+1 < audio_->analysed_data().intervals_.size())
    cur_interval_ = audio_->analysed_data().intervals_[cur_interval_.id_+1];
}

void AudioKi::SetBattleTactics() {
  // Major: defence
  if (cur_interval_.major_) {
    // Additionally increase depending on signature.
    if (cur_interval_.signature_ == Signitue::SHARP)
      defence_strategies_[Tactics::DEF_FRONT_FOCUS] += 2;
    else if (cur_interval_.signature_ == Signitue::FLAT)
      defence_strategies_[Tactics::DEF_SURROUNG_FOCUS] += 2;
    else
      defence_strategies_[Tactics::DEF_IPSP_BLOCK] += 2;
  }
  // Minor: attack
  else {
    building_tactics_[SYNAPSE] += 5;
    // Epsp/ ipsp depending on signature
    if (cur_interval_.signature_ == Signitue::UNSIGNED)
      attack_strategies_[Tactics::EPSP_FOCUSED] += 2;
    else
      attack_strategies_[Tactics::IPSP_FOCUSED] += 2;
    // targets/ brute force/ intelligent way depending on signature
    if (cur_interval_.signature_ == Signitue::FLAT) {  // targets
      if (cur_interval_.darkness_ > 4)
        attack_strategies_[DESTROY_ACTIVATED_NEURONS] += 2;
      else if (cur_interval_.darkness_ < 4) 
        attack_strategies_[DESTROY_ACTIVATED_NEURONS] += 2;
      if (cur_interval_.notes_out_key_ > 4)
        attack_strategies_[BLOCK_ACTIVATED_NEURON] += 2;
      else 
        attack_strategies_[BLOCK_SYNAPSES] += 2;
    }
    else if (cur_interval_.signature_ == Signitue::UNSIGNED) // brute force
      attack_strategies_[AIM_NUCLEUS] += 3;
    // TODO (fux): intelligent way.
  }

  // Add more random factors: key_note, darkness and notes outside of key.
  for (const auto& factor : {cur_interval_.key_note_, cur_interval_.darkness_, cur_interval_.notes_out_key_}) {
    attack_strategies_.at(factor % attack_strategies_.size())++;
    defence_strategies_.at(factor % defence_strategies_.size() + DEF_FRONT_FOCUS)++;
    building_tactics_.at(factor % building_tactics_.size())++;
  }

  // Get final ipsp and epsp target strategies:
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

  // Log results for this interval.
  spdlog::get(LOGGER)->info("key {}, darkness {}, notes_out_key {}", cur_interval_.key_, cur_interval_.darkness_, 
      cur_interval_.notes_out_key_);
  for (const auto& it : attack_strategies_)
    spdlog::get(LOGGER)->info("{}: {}", tactics_mapping.at(it.first), it.second);
  for (const auto& it : defence_strategies_)
    spdlog::get(LOGGER)->info("{}: {}", tactics_mapping.at(it.first), it.second);
  for (const auto& it : building_tactics_)
    spdlog::get(LOGGER)->info("{}: {}", units_tech_mapping.at(it.first), it.second);
}

void AudioKi::SetEconomyTactics() {
  std::map<size_t, size_t> resource_tactics;
  std::map<size_t, size_t> technology_tactics;

  // Resources direct
  unsigned atk_boast = (cur_interval_.major_) ? 0 : 10;
  resource_tactics[CHLORIDE] = atk_boast + attack_strategies_[IPSP_FOCUSED] + defence_strategies_[DEF_IPSP_BLOCK]
    + attack_strategies_[BLOCK_ACTIVATED_NEURON] + attack_strategies_[BLOCK_SYNAPSES];
  resource_tactics[POTASSIUM] = atk_boast + attack_strategies_[EPSP_FOCUSED] + attack_strategies_[DESTROY_SYNAPSES]
    + attack_strategies_[DESTROY_ACTIVATED_NEURONS];
  unsigned def_boast = (cur_interval_.major_) ? 10 : 0;
  resource_tactics[GLUTAMATE] = 4 + def_boast + (defence_strategies_[DEF_FRONT_FOCUS] 
      + defence_strategies_[DEF_SURROUNG_FOCUS])/2;
  resource_tactics[DOPAMINE] = atk_boast + defence_strategies_[DEF_IPSP_BLOCK];
  if (def_boast > atk_boast) {
    resource_tactics[DOPAMINE] = resource_tactics[GLUTAMATE]-1;
    resource_tactics[SEROTONIN] = resource_tactics[GLUTAMATE]-2;
  }
  // Get sorted list of resource_tactics and add resources in sorted order.
  for (const auto& it : SortStrategy(resource_tactics)) 
    resource_tactics_.push_back(it.second);
  // Distribute extra iron +3 top resources, +2 second resource, +1 third resource.
  for (unsigned int i : {0, 0, 0, 1, 1, 2})
    resource_tactics_.push_back(resource_tactics_[i]);
  // Log final resource tactics.
  for (const auto& it : resource_tactics_)
    spdlog::get(LOGGER)->info("resource: {}", resources_name_mapping.at(it));
   
  // technologies
  technology_tactics[SWARM] = (attack_strategies_[AIM_NUCLEUS] > 2) ? 5 : 0;
  technology_tactics[TARGET] = ((attack_strategies_[AIM_NUCLEUS] > 2) ? 0 : 5) + defence_strategies_[DEF_IPSP_BLOCK];
  if (cur_interval_.major_)
    for (const auto& it : {DEF_POTENTIAL, DEF_SPEED})
      technology_tactics[it] = 3*((cur_interval_.signature_ == SHARP) ? 2 : 1); // additional refinment boast.
  else {
    for (const auto& it : {ATK_POTENIAL, ATK_SPEED, ATK_DURATION})
      technology_tactics[it] = 2*((cur_interval_.signature_ == SHARP) ? 2 : 1); // additional refinment boast.
  }
  // resources-focues technologies.
  technology_tactics[CURVE] = (cur_interval_.signature_ == FLAT) ? 9 : 0;  
  technology_tactics[TOTAL_RESOURCE] = (cur_interval_.signature_ == FLAT) ? 9 : 0;  // resources
  // expanssion focuesed.
  technology_tactics[NUCLEUS_RANGE] = (!cur_interval_.major_ && cur_interval_.signature_ != Signitue::UNSIGNED) ? 9 : 0;
  // TODO (fux): handle technology_tactics[WAY]
  // Added sorted technology tacics to final tactics three times (as there are three levels for each technology).
  for (unsigned int i=0; i<3; i++) 
    for (const auto& it : SortStrategy(technology_tactics)) 
      technology_tactics_.push_back(it.second);
  // log final technology tactics
  for (const auto& it : resource_tactics_)
    spdlog::get(LOGGER)->info("resource: {}", resources_name_mapping.at(it));

  // building tactics.
  if (cur_interval_.major_)
    building_tactics_[ACTIVATEDNEURON] += 10;
  else
    building_tactics_[SYNAPSE] += 4;
}

void AudioKi::DoAction(const AudioDataTimePoint& data_at_beat) {
  spdlog::get(LOGGER)->debug("AudioKi::DoAction.");
  // Change tactics when interval changes:
  if (data_at_beat.interval_ > last_data_point_.interval_)
    SetUpTactics(false);

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
  if (last_data_point_.level_ >= average_level_ && data_at_beat.level_ < average_level_)
    CreateActivatedNeuron();

  if (audio_->MoreOffNotes(data_at_beat, false))
    NewTechnology(data_at_beat);
  else 
    HandleIron(data_at_beat);
  CreateExtraActivatedNeurons();

  spdlog::get(LOGGER)->info("Enemy current resources: {}", GetCurrentResources());
}

void AudioKi::LaunchAttack(const AudioDataTimePoint& data_at_beat) {
  spdlog::get(LOGGER)->debug("AudioKi::LaunchAttack.");
  // Sort synapses (use synapses futhest from enemy for epsp)
  auto sorted_synapses = SortPositionsByDistance(enemy_->nucleus_pos(), GetAllPositionsOfNeurons(UnitsTech::SYNAPSE));
  if (sorted_synapses.size() == 0)
    return;
  std::unique_lock ul(mutex_all_neurons_);
  spdlog::get(LOGGER)->debug("AudioKi::LaunchAttack: get epsp synapses.");
  position_t epsp_synapses_pos = sorted_synapses.back();
  auto epsp_way = field_->GetWayForSoldier(epsp_synapses_pos, neurons_.at(epsp_synapses_pos)->GetWayPoints(UnitsTech::EPSP));
  ul.unlock();
 
  spdlog::get(LOGGER)->debug("AudioKi::LaunchAttack: Get ipsp targets");
  auto ipsp_launch_synapes = AvailibleIpspLaunches(sorted_synapses, 5);
  auto ipsp_targets = GetIpspTargets(epsp_way, sorted_synapses);  // using epsp-way, since we want to clear this way.
  spdlog::get(LOGGER)->debug("AudioKi::LaunchAttack: Got {} ipsp targets.", ipsp_targets.size());
  spdlog::get(LOGGER)->debug("AudioKi::LaunchAttack: Get epsp target");
  position_t epsp_target = {-1, -1};
  // Take first target which is not already ipsp target.
  auto possible_epsp_targets = GetEpspTargets(sorted_synapses.back(), epsp_way);
  for (const auto& it : possible_epsp_targets)
    if (std::find(ipsp_targets.begin(), ipsp_targets.end(), it) == ipsp_targets.end())
      epsp_target = it;
  if (epsp_target.first == -1 && epsp_target.second == -1 && possible_epsp_targets.size() > 0)
    epsp_target = possible_epsp_targets.back();

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

std::vector<position_t> AudioKi::GetEpspTargets(position_t synapse_pos, std::list<position_t> way, size_t ignore_strategy) {
  spdlog::get(LOGGER)->debug("AudioKi::GetEpspTargets");
  if (technologies_.at(UnitsTech::TARGET).first < 2)  {
    spdlog::get(LOGGER)->debug("AudioKi::GetEpspTargets: using default.");
    return {enemy_->nucleus_pos()};
  }
  if (epsp_target_strategy_ == DESTROY_ACTIVATED_NEURONS && ignore_strategy != DESTROY_ACTIVATED_NEURONS) {
    spdlog::get(LOGGER)->debug("AudioKi::GetEpspTargets: using DESTROY_ACTIVATED_NEURONS.");
    auto activated_neurons_on_way = GetAllActivatedNeuronsOnWay(way);
    activated_neurons_on_way = SortPositionsByDistance(nucleus_pos_, activated_neurons_on_way, false);
    if (activated_neurons_on_way.size() == 0)
      return GetEpspTargets(synapse_pos, way, DESTROY_ACTIVATED_NEURONS);
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

std::vector<position_t> AudioKi::GetIpspTargets(std::list<position_t> way, std::vector<position_t>& synapses, size_t ignore_strategy) {
  spdlog::get(LOGGER)->debug("AudioKi::GetIpspTargets");
  if (technologies_.at(UnitsTech::TARGET).first == 0) {
    spdlog::get(LOGGER)->debug("AudioKi::GetIpspTargets: using default.");
    return {enemy_->GetRandomNeuron()};
  }
  std::vector<position_t> isps_targets;
  if (ipsp_target_strategy_ == BLOCK_ACTIVATED_NEURON && ignore_strategy != BLOCK_ACTIVATED_NEURON) {
    spdlog::get(LOGGER)->debug("AudioKi::GetIpspTargets: using BLOCK_ACTIVATED_NEURON.");
    auto activated_neurons_on_way = GetAllActivatedNeuronsOnWay(way);
    activated_neurons_on_way = SortPositionsByDistance(nucleus_pos_, activated_neurons_on_way, false);
    for (size_t i=0; i<synapses.size() && i<activated_neurons_on_way.size(); i++)
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

void AudioKi::CreateEpsps(position_t synapse_pos, position_t target_pos, int bpm) {
  spdlog::get(LOGGER)->debug("AudioKi::CreateEpsp.");
  ChangeEpspTargetForSynapse(synapse_pos, target_pos);
  // Calculate update number of epsps to create and update interval
  double update_interval = 60000.0/(bpm*16);
  auto last_epsp = std::chrono::steady_clock::now();  
  while (GetMissingResources(UnitsTech::EPSP).size() == 0) {
    // Add epsp in certain interval.
    if (utils::GetElapsed(last_epsp, std::chrono::steady_clock::now()) < update_interval) {
      AddPotential(synapse_pos, UnitsTech::EPSP);
      last_epsp = std::chrono::steady_clock::now();
    }
  }
}

void AudioKi::CreateIpsps(position_t synapse_pos, position_t target_pos, int num_ipsp_to_create, int bpm) {
  spdlog::get(LOGGER)->debug("AudioKi::CreateIpsp.");
  ChangeIpspTargetForSynapse(synapse_pos, target_pos);

  // Calculate update number of ipsps to create and update interval
  double update_interval = 60000.0/(bpm*16);
  auto last_ipsp = std::chrono::steady_clock::now();  
  while (GetMissingResources(UnitsTech::IPSP).size() == 0) {
    // Add ipsp in certain interval.
    if (utils::GetElapsed(last_ipsp, std::chrono::steady_clock::now()) >= update_interval) {
      AddPotential(synapse_pos, UnitsTech::IPSP);
      last_ipsp = std::chrono::steady_clock::now();
    }
  }
}

void AudioKi::CreateSynapses(bool force) {
  spdlog::get(LOGGER)->debug("AudioKi::Synapse.");
  int availible_oxygen = resources_.at(OXYGEN).limit() - resources_.at(OXYGEN).bound();
  if (GetAllPositionsOfNeurons(UnitsTech::SYNAPSE).size() <= building_tactics_[SYNAPSE] && availible_oxygen > 25) {
    spdlog::get(LOGGER)->debug("AudioKi::CreateSynapses: creating synapses.");
    auto pos = field_->FindFree(nucleus_pos_.first, nucleus_pos_.second, 1, 5);
    if (AddNeuron(pos, UnitsTech::SYNAPSE, enemy_->nucleus_pos())) {
      field_->AddNewUnitToPos(pos, UnitsTech::SYNAPSE);
      CheckResourceLimit();
    }
    spdlog::get(LOGGER)->debug("AudioKi::CreateSynapses: done.");
  }
}

void AudioKi::CreateActivatedNeuron(bool force) {
  spdlog::get(LOGGER)->debug("AudioKi::CreateActivatedNeuron.");
  size_t num_activated_neurons = GetAllPositionsOfNeurons(ACTIVATEDNEURON).size();
  int availible_oxygen = resources_.at(OXYGEN).limit() - resources_.at(OXYGEN).bound();
  if (!force && (num_activated_neurons >= building_tactics_[ACTIVATEDNEURON] || availible_oxygen < 25))
    return;
  // Don't add def, if no synapses already exists and atleast one def already exists.
  if (!force && num_activated_neurons > 0 && GetAllPositionsOfNeurons(UnitsTech::SYNAPSE).size() == 0)
    return;

  spdlog::get(LOGGER)->debug("AudioKi::CreateActivatedNeuron: createing neuron");
  // Find position to place neuron coresponding to tactics.
  position_t pos;
  if (SortStrategy(defence_strategies_).front().second == DEF_SURROUNG_FOCUS) {
    for (int i=1; i<cur_range_; i++) {
      auto positions = field_->GetAllInRange(nucleus_pos_, i, i-1, true);
      if (positions.size() > 0) {
        positions = SortPositionsByDistance(enemy_->nucleus_pos(), positions);
        pos = positions.front();
        break;
      }
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
        if (utils::Dist(position, way_point) <= 3)
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
    CheckResourceLimit();
    spdlog::get(LOGGER)->debug("AudioKi::CreateActivatedNeuron: created activated neuron.");
  }
  else
    spdlog::get(LOGGER)->debug("AudioKi::CreateActivatedNeuron: not enough resources");
}

void AudioKi::HandleIron(const AudioDataTimePoint& data_at_beat) {
  spdlog::get(LOGGER)->debug("AudioKi::HandleIron.");

  std::shared_lock sl(mutex_resources_);
  unsigned int iron = resources_.at(IRON).cur();
  sl.unlock();

  // Check if empty.
  if (resource_tactics_.empty() || iron == 0 || (cur_interval_.id_ > 0 && iron < 2))
    return;
  size_t resource = resource_tactics_.front();
  if (!DistributeIron(resource)) {
    spdlog::get(LOGGER)->debug("AudioKi::HandleIron: no iron or other error.");
    return;
  }
  // If resource is now activated, procceed to next resource.
  sl.lock();
  if (resources_.at(resource).Active())
    resource_tactics_.erase(resource_tactics_.begin());
  // If not activated and iron left, distribute again.
  else if (resources_.at(IRON).cur() > 0) {
    sl.unlock();
    HandleIron(data_at_beat);
  }
  spdlog::get(LOGGER)->debug("AudioKi::HandleIron: done.");
}

void AudioKi::NewTechnology(const AudioDataTimePoint& data_at_beat) {
  spdlog::get(LOGGER)->debug("AudioKi::NewTechnology.");

  // Check if empty.
  if (technology_tactics_.empty()) {
    spdlog::get(LOGGER)->debug("AudioKi::NewTechnology: List empty.");
    return;
  }
  // Research technology and remove from tech-list.
  size_t technology = technology_tactics_.front();
  if (!AddTechnology(technology)) {
    spdlog::get(LOGGER)->debug("AudioKi::NewTechnology: no resources or other error.");
    return;
  }
  technology_tactics_.erase(technology_tactics_.begin());
  // If technology was already fully researched, research next technology right away.
  std::shared_lock sl(mutex_technologies_);
  if (technologies_.at(technology).first == technologies_.at(technology).second) {
    spdlog::get(LOGGER)->debug("AudioKi::NewTechnology: calling again, as fully researched.");
    sl.unlock();
    sl.release();
    NewTechnology(data_at_beat);
  }
  spdlog::get(LOGGER)->debug("AudioKi::NewTechnology: success.");
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
  size_t res = std::min(resources_.at(POTASSIUM).cur() / costs[POTASSIUM], resources_.at(CHLORIDE).cur() / costs[CHLORIDE]);
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
  size_t res = resources_.at(POTASSIUM).cur() / (costs[POTASSIUM] + ipsps_to_create*costs_ipsp[POTASSIUM]);
  return res;
}


std::vector<position_t> AudioKi::AvailibleIpspLaunches(std::vector<position_t>& synapses, int min) {
  spdlog::get(LOGGER)->debug("AudioKi::AvailibleIpspLaunches.");
  size_t available_ipsps = AvailibleIpsps();  
  std::vector<position_t> result_positions;
  for (size_t i=0; i<available_ipsps; i+=min)
    result_positions.push_back(synapses[i]);
  spdlog::get(LOGGER)->debug("AudioKi::AvailibleIpspLaunches: got {} launch positions", result_positions.size());
  return result_positions;
}

std::vector<position_t> AudioKi::GetAllActivatedNeuronsOnWay(std::list<position_t> way) {
  spdlog::get(LOGGER)->debug("AudioKi::GetAllActivatedNeuronsOnWay.");
  auto enemy_activated_neurons = enemy_->GetAllPositionsOfNeurons(UnitsTech::ACTIVATEDNEURON);
  std::vector<position_t> result_positions;
  for (const auto& way_point : way) {
    for (const auto& activated_neuron_pos : enemy_activated_neurons) {
      if (utils::Dist(way_point, activated_neuron_pos) < 3)
        result_positions.push_back(activated_neuron_pos);
    }
  }
  spdlog::get(LOGGER)->debug("AudioKi::GetAllActivatedNeuronsOnWay: got {} activated neurons on way.", result_positions.size());
  return result_positions;
}

std::vector<position_t> AudioKi::SortPositionsByDistance(position_t start, std::vector<position_t> positions, bool reverse) {
  spdlog::get(LOGGER)->debug("AudioKi::SortPositionsByDistance.");
  std::list<std::pair<size_t, position_t>> sorted_positions;
  for (const auto& it : positions)
    sorted_positions.push_back({utils::Dist(start, it), it});
  sorted_positions.sort();
  if (reverse)
    sorted_positions.reverse();
  std::vector<position_t> result_positions;
  for (const auto& it : sorted_positions)
    result_positions.push_back(it.second);
  spdlog::get(LOGGER)->debug("AudioKi::SortPositionsByDistance: done.");
  return result_positions; 
}

std::vector<position_t> AudioKi::GetEnemySynapsesSortedByLeastDef(position_t start) {
  spdlog::get(LOGGER)->debug("AudioKi::GetEnemySynapsesSortedByLeastDef.");
  auto enemy_synapses = enemy_->GetAllPositionsOfNeurons(UnitsTech::SYNAPSE);
  std::list<std::pair<size_t, position_t>> sorted_positions;
  for (const auto& it : enemy_synapses) {
    auto way = field_->GetWayForSoldier(start, {it});
    sorted_positions.push_back({GetAllActivatedNeuronsOnWay(way).size(), it});
  }
  sorted_positions.sort();
  sorted_positions.reverse();
  std::vector<position_t> result_positions;
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
  double diff = resources_.at(POTASSIUM).limit() * 0.6 - num_epsps_to_create * costs_epsp[POTASSIUM]; // max-to-spend -                                                                                                       // total costs.
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
  while (utils::GetElapsed(start, std::chrono::steady_clock::now()) < wait_time) { continue; }
  spdlog::get(LOGGER)->info("AudioKi::SynchAttacks: Done waiting.");
}

void AudioKi::CheckResourceLimit() {
  std::shared_lock sl (mutex_resources_);
  for (const auto& it : resources_) {
    double procent_full = (it.second.cur()+it.second.bound())/it.second.limit();
    if (procent_full >= 0.8) {
      sl.unlock();
      AddTechnology(UnitsTech::TOTAL_RESOURCE);
      break;
    }
  }
}

void AudioKi::CreateExtraActivatedNeurons() {
  spdlog::get(LOGGER)->info("AudioKi::CreateExtraActivatedNeurons");
  // Build activated neurons based on current voltage.
  if (extra_activated_neurons_.count(nucleus_.voltage()) > 0 && extra_activated_neurons_.at(nucleus_.voltage()) > 0) {
    spdlog::get(LOGGER)->info("AudioKi::CreateExtraActivatedNeurons builiding {} a-neurons based on current voltag.",
     extra_activated_neurons_.at(nucleus_.voltage()));
    CreateActivatedNeuron(true);
    extra_activated_neurons_.at(nucleus_.voltage())--;
  }

  // Build activated neurons based on enemy epsp-attack launch.
  unsigned int enemy_potentials = 0;
  for (const auto& it : enemy_->potential()) 
    if (it.second.type_ == EPSP) 
      enemy_potentials++;
  if (enemy_potentials > 0) {
    auto way = enemy_->potential().begin()->second.way_;
    int diff = GetAllActivatedNeuronsOnWay(way).size()*3-enemy_potentials;
    spdlog::get(LOGGER)->info("AudioKi::CreateExtraActivatedNeurons got missing defs: {}/3", diff);
    if (diff > 0) {
      diff /= 3;
      while(--diff > 0)
        CreateActivatedNeuron(true);
    }
  }
}
