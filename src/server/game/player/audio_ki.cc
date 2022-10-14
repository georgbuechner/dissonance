#include "server/game/player/audio_ki.h" 
#include "share/audio/audio.h"
#include "share/constants/codes.h"
#include "share/constants/costs.h"
#include "server/game/field/field.h"
#include "share/defines.h"
#include "share/objects/units.h"
#include "share/tools/utils/utils.h"

#include "spdlog/spdlog.h"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iterator>
#include <algorithm>
#include <memory>
#include <numeric>
#include <string>
#include <vector>

#define ENEMY_ACTIAVTED_NEURON_THRESHOLD 6

AudioKi::AudioKi(std::string username, std::shared_ptr<Field> field, std::shared_ptr<Audio> audio, 
    std::shared_ptr<RandomGenerator> ran_gen, int color) 
    : Player(username, field, ran_gen, color), average_level_(audio->analysed_data().average_level_) {
  // Set up Audio
  audio_ = audio;
  cur_interval_ = audio_->analysed_data().intervals_[0];
  audio_beats_ = audio_->analysed_data().data_per_beat_;

  // Set up tactics
  epsp_target_strategies_ = {{AIM_NUCLEUS, 3}, {DESTROY_ACTIVATED_NEURONS, 1}, {DESTROY_SYNAPSES, 1}, 
    {DESTROY_RESOURCES, 1}};
  ipsp_target_strategies_ = {{BLOCK_ACTIVATED_NEURON, 1}, {BLOCK_SYNAPSES, 1}, {BLOCK_RESOURCES, 1}};
  ipsp_epsp_strategies_ = {{EPSP_FOCUSED, 1}, {IPSP_FOCUSED, 1}};
  activated_neuron_strategies_ = {{DEF_SURROUNG_FOCUS, 1}, {DEF_FRONT_FOCUS, 1}};
  def_strategies_ = {{DEF_IPSP_BLOCK, 1}, {DEF_AN_BLOCK, 1}};
  building_tactics_ = {{ACTIVATEDNEURON, 4}, {SYNAPSE, 1}, {NUCLEUS, 1}};
}

std::deque<AudioDataTimePoint> AudioKi::audio_beats() const {
  return audio_beats_;
}

std::map<std::string, size_t> AudioKi::strategies() const {
  std::map<std::string, size_t> strategies;
  strategies["Epsp target strategy: "] = GetTopStrategy(epsp_target_strategies_);
  strategies["Ipsp target strategy: "] = GetTopStrategy(ipsp_target_strategies_);
  strategies["Activated neuron strategy: "] = GetTopStrategy(activated_neuron_strategies_);
  strategies["Main defence strategy: "] = GetTopStrategy(def_strategies_);
  strategies["Epsp-/ Ipsp-focused: "] = GetTopStrategy(ipsp_epsp_strategies_);
  if (audio_->analysed_data().Allegro())
    strategies["Attack based"] = 0xFFF;
  else 
    strategies["Defence based"] = 0xFFF;
  return strategies;
}

void AudioKi::Setup() {
  // Setup tactics
  SetupTactics(true);
  // Distribute initial iron.
  DistributeIron(Resources::OXYGEN);
  DistributeIron(Resources::OXYGEN);
  resources_activated_.insert(OXYGEN);
  HandleIron();
}

void AudioKi::SetupTactics(bool inital_setup) {
  spdlog::get(LOGGER)->info("AudioKi::SetUpTactics");
  // Setup tactics.
  SetBattleTactics();
  // If initial setup, also set up economy tactics and distribite initial resources.
  if (inital_setup)
    SetEconomyTactics();
  // Increase interval.
  if ((size_t)cur_interval_.id_+1 < audio_->analysed_data().intervals_.size())
    cur_interval_ = audio_->analysed_data().intervals_[cur_interval_.id_+1];
}

void AudioKi::SetBattleTactics() {
  spdlog::get(LOGGER)->info("AudioKi::SetBattleTactics");
  // IPSP/EPSP Focus (key-note)
  if (cur_interval_.major_) {
    ipsp_target_strategies_[Tactics::IPSP_FOCUSED] += 2;
    def_strategies_[Tactics::DEF_IPSP_BLOCK] += 4;
  }
  else {
    ipsp_epsp_strategies_[Tactics::EPSP_FOCUSED] += 2;
    def_strategies_[Tactics::DEF_AN_BLOCK] += 4;
  }
  // IPSP-Target-Strategies: UNSIGNED->AIM_NUCLEUS, SHARP->DESTROY_RESOURCES, FLAT->DESTROY_SYNAPSES
  //  destroy-activated-neurons only relevant for random factors and added if
  //  enemy has large amount of activated-neurons
  epsp_target_strategies_[AIM_NUCLEUS + cur_interval_.signature_] += 3;
  // IPSP-Target-Strategies: UNSIGNED->BLOCK_ACTIVATED_NEURON, SHARP->BLOCK_SYNAPSES, FLAT->BLOCK_RESOURCES
  ipsp_target_strategies_[BLOCK_ACTIVATED_NEURON + cur_interval_.signature_] += 3;

  // ACTIVATED NEURONS-strategy
  if (cur_interval_.notes_out_key_ > 4)
    activated_neuron_strategies_[DEF_FRONT_FOCUS] += 2;
  else 
    activated_neuron_strategies_[Tactics::DEF_SURROUNG_FOCUS] += 2;

  // BUILDING TACTICS:
  if (audio_->analysed_data().Allegro())
    building_tactics_[SYNAPSE] += 4;
  else
    building_tactics_[ACTIVATEDNEURON] += 10;

  // Add more random factors: key_note, darkness and notes outside of key.
  for (const auto& factor : {cur_interval_.key_note_, cur_interval_.darkness_, cur_interval_.notes_out_key_}) {
    epsp_target_strategies_.at(AIM_NUCLEUS + factor % epsp_target_strategies_.size())++;
    ipsp_target_strategies_.at(BLOCK_ACTIVATED_NEURON + factor % ipsp_epsp_strategies_.size())++;
    activated_neuron_strategies_.at(DEF_FRONT_FOCUS + factor % activated_neuron_strategies_.size())++;
    ipsp_epsp_strategies_.at(factor % ipsp_epsp_strategies_.size())++;
    building_tactics_.at(factor % building_tactics_.size())++;
  }

  // Log results for this interval.
  for (const auto& it : epsp_target_strategies_)
    spdlog::get(LOGGER)->debug("{}: {}", tactics_mapping.at(it.first), it.second);
  for (const auto& it : ipsp_target_strategies_)
    spdlog::get(LOGGER)->debug("{}: {}", tactics_mapping.at(it.first), it.second);
  for (const auto& it : def_strategies_)
    spdlog::get(LOGGER)->debug("{}: {}", tactics_mapping.at(it.first), it.second);
  for (const auto& it : activated_neuron_strategies_)
    spdlog::get(LOGGER)->debug("{}: {}", tactics_mapping.at(it.first), it.second);
  for (const auto& it : ipsp_epsp_strategies_)
    spdlog::get(LOGGER)->debug("{}: {}", tactics_mapping.at(it.first), it.second);
  for (const auto& it : building_tactics_)
    spdlog::get(LOGGER)->debug("{}: {}", units_tech_name_mapping.at(it.first), it.second);
}

void AudioKi::SetEconomyTactics() {
  spdlog::get(LOGGER)->info("AudioKi::SetEconomyTactics");
  std::map<size_t, size_t> technology_tactics;

  // Resources attack based: potassium focus
  if (audio_->analysed_data().Allegro()) {
    resource_tactics_.push_back(POTASSIUM);
    if (GetTopStrategy(def_strategies_) == DEF_IPSP_BLOCK)
      resource_tactics_.insert(resource_tactics_.end(), {CHLORIDE, GLUTAMATE});
    else
      resource_tactics_.insert(resource_tactics_.end(), {GLUTAMATE, CHLORIDE});
    resource_tactics_.insert(resource_tactics_.end(), {DOPAMINE, SEROTONIN});
  }
  // Resources attack based: glutamatefocus
  else {
    if (GetTopStrategy(def_strategies_) == DEF_IPSP_BLOCK)
      resource_tactics_ = {CHLORIDE, GLUTAMATE, POTASSIUM};
    else
      resource_tactics_ = {GLUTAMATE, POTASSIUM, CHLORIDE};
    resource_tactics_.insert(resource_tactics_.end(), {SEROTONIN, DOPAMINE});
  }
  // Distribute extra iron +3 top resources, +2 second resource, +1 third resource.
  for (int i : {0, 0, 0, 1, 1, 2})
    resource_tactics_.push_back(resource_tactics_[i]);
  // Log final resource tactics.
  for (const auto& it : resource_tactics_)
    spdlog::get(LOGGER)->debug("resource: {}", resources_name_mapping.at(it));
   
  // technologies
  technology_tactics[WAY] = (GetTopStrategy(def_strategies_) == DEF_IPSP_BLOCK) ? 10 : 2;
  technology_tactics[SWARM] = (GetTopStrategy(epsp_target_strategies_) == AIM_NUCLEUS) ? 5 : 2;
  if (audio_->analysed_data().Allegro())
    for (const auto& it : {ATK_POTENIAL, ATK_SPEED, ATK_DURATION, WAY})
      technology_tactics[it] = 2*((cur_interval_.signature_ == SHARP) ? 3 : 1); // additional refinment boast.
  else
    for (const auto& it : {DEF_POTENTIAL, DEF_SPEED})
      technology_tactics[it] = 3*((cur_interval_.signature_ == SHARP) ? 3 : 1); // additional refinment boast.
  // resources-focues technologies.
  technology_tactics[CURVE] = (cur_interval_.signature_ == FLAT) ? cur_interval_.darkness_%9 : 0;  
  technology_tactics[TOTAL_RESOURCE] = (cur_interval_.signature_ == FLAT) ? cur_interval_.darkness_%9 : 0;  // resources
  // expanssion focuesed.
  technology_tactics[NUCLEUS_RANGE] = (audio_->analysed_data().Allegro() 
      && cur_interval_.signature_ != Signitue::UNSIGNED) ? cur_interval_.darkness_%9 : 0;
  // Added sorted technology tacics to final tactics three times (as there are three levels for each technology).
  for (int i=0; i<3; i++) 
    for (const auto& it : SortStrategy(technology_tactics)) 
      technology_tactics_.push_back(it.second);
  // log final technology tactics
  for (const auto& it : technology_tactics_)
    spdlog::get(LOGGER)->debug("technology: {}", units_tech_name_mapping.at(it));
}

bool AudioKi::DoAction() {
  spdlog::get(LOGGER)->debug("AudioKi::DoAction beats left: {}.", audio_beats_.size());
  auto next_data_at_beat = audio_beats_.front();
  audio_beats_.pop_front();
  DoAction(next_data_at_beat);
  last_beat_ = next_data_at_beat;
  // reload
  if (audio_beats_.size() == 0) {
    audio_beats_ = audio_->analysed_data().data_per_beat_;
    return true;
  }
  return false;
}

bool AudioKi::DoAction(const AudioDataTimePoint& beat) {
  spdlog::get(LOGGER)->debug("AudioKi::DoAction(beat): level {}, average_level: {}",
      beat.level_, average_level_);
  // Change tactics when interval changes:
  if (beat.interval_ > last_beat_.interval_)
    SetupTactics(false);
  if (beat.level_ > average_level_) {
    // Create synapses when switch above average level.
    if (last_level_peaks_.size() == 0) 
      CreateSynapses();
    // Add data_at_beat to list of beats since switching above average.
    last_level_peaks_.push_back(beat.level_-average_level_);
  }
  // Launch attack, when above average wave is over.
  if (beat.level_ <= average_level_ && last_level_peaks_.size() > 0) {
    LaunchAttack();
    last_level_peaks_.clear();
  }
  // Create activated neuron if level drops below average.
  if (last_beat_.level_ >= average_level_ && beat.level_ < average_level_)
    CreateActivatedNeuron();
  // Check for destroyed resource-neurons:
  for (const auto& it : resources_activated_) {
    if (!resources_.at(it).Active()) {
      DistributeIron(it);
      DistributeIron(it);
    }
  }
  // Technologies and iron distribution:
  HandleHighBound();
  if (audio_->MoreOffNotes(beat, false))
    NewTechnology();
  else 
    HandleIron();

  Defend();
  spdlog::get(LOGGER)->debug("AudioKi::DoAction(beat) done.");
  return false;
}

void AudioKi::LaunchAttack() {
  spdlog::get(LOGGER)->debug("AudioKi::LaunchAttack.");
  // Sort synapses (use synapses futhest from enemy for epsp)
  auto sorted_synapses = SortPositionsByDistance(enemies_.front()->GetOneNucleus(), GetAllPositionsOfNeurons(SYNAPSE));
  if (sorted_synapses.size() == 0)
    return;
  position_t epsp_synapses_pos = sorted_synapses.back();
  // Needed for some aproxiamte calculations of enemies defence.
  auto epsp_way = field_->GetWay(epsp_synapses_pos, neurons_.at(epsp_synapses_pos)->GetWayPoints(UnitsTech::EPSP));
  auto enemy_activated_neuerons = enemies_.front()->GetAllPositionsOfNeurons(ACTIVATEDNEURON);
  auto activated_neurons_on_way = GetAllActivatedNeuronsOnWay(enemy_activated_neuerons, epsp_way);
  // If a lot of activated neurons are on way, make destroying them primary epsp-target-strategy
  if (activated_neurons_on_way.size() > ENEMY_ACTIAVTED_NEURON_THRESHOLD 
      && GetTopStrategy(epsp_target_strategies_) != DESTROY_ACTIVATED_NEURONS)
    epsp_target_strategies_.at(DESTROY_ACTIVATED_NEURONS) += 10;
  // Otherwise make this least favarobale strategy.
  else if (enemy_activated_neuerons.size() <= ENEMY_ACTIAVTED_NEURON_THRESHOLD 
      && GetTopStrategy(epsp_target_strategies_) == DESTROY_ACTIVATED_NEURONS)
    epsp_target_strategies_.at(DESTROY_ACTIVATED_NEURONS) = 0;

  // get ipsp- and epsp-target
  auto ipsp_launch_target_pair = GetIpspLaunchesAndTargets(epsp_way, sorted_synapses);  
  position_t epsp_target = GetEpspTarget(epsp_synapses_pos, epsp_way, ipsp_launch_target_pair);

  // Check whether to launch attack.
  size_t available_ipsps = AvailibleIpsps();
  size_t num_epsps_to_create = NumberOfEpspsToCreate(epsp_way, available_ipsps);

  // Launch ipsp attacks first. Only launch if epsp attack is launched too or
  // strategy is blocking enemy synapses.
  if (GetTopStrategy(ipsp_target_strategies_) != BLOCK_ACTIVATED_NEURON || num_epsps_to_create > 0) {
    size_t available_ipsps = AvailibleIpsps();
    for (const auto& it : ipsp_launch_target_pair) {
      int first = 0;
      for (const auto& waypoint : FindBestWayPoints(it.first, it.second))
        AddWayPosForSynapse(it.first, waypoint, first++==0);
      CreateIpsps(it.first, it.second, available_ipsps/ipsp_launch_target_pair.size());
    }
  }
  // Only launch if expected number of epsps to create is reached.
  if (num_epsps_to_create > 0) {
    int inital_speed_decrease=0;
    // Synch attack if ipsps where created (launch synapse and targets where found)
    if (ipsp_launch_target_pair.size() > 0) {
      auto ipsp_way = field_->GetWay(ipsp_launch_target_pair.front().first, 
          neurons_.at(ipsp_launch_target_pair.front().first)->GetWayPoints(UnitsTech::IPSP));
      inital_speed_decrease = SynchAttacks(epsp_way.size(), ipsp_way.size());
    }
    // Launch epsps next.
    int first = 0;
    for (const auto& waypoint : FindBestWayPoints(sorted_synapses.back(), epsp_target))
      AddWayPosForSynapse(sorted_synapses.back(), waypoint, first++==0);
    CreateEpsps(sorted_synapses.back(), epsp_target, num_epsps_to_create, inital_speed_decrease);
  }
}

position_t AudioKi::GetEpspTarget(position_t synapse_pos, const std::list<position_t>& way, 
        const std::vector<launch_target_pair_t>& ipsp_launch_target_pairs) const {
  auto possible_epsp_targets = GetEpspTargets(synapse_pos, way);
  // Ideally select target which is not already ipsp-target.
  for (const auto& epsp_target : possible_epsp_targets) {
    bool found = false;
    for (const auto& ipsp_launch_target_pair : ipsp_launch_target_pairs) {
      if (ipsp_launch_target_pair.second != epsp_target) {
        found = true;
        break;
      }
      if (!found)
        return epsp_target;
    }
  }
  return possible_epsp_targets.front();
}

std::vector<position_t> AudioKi::GetEpspTargets(position_t synapse_pos, const std::list<position_t>& way, 
    size_t ignore_strategy) const {
  size_t epsp_target_strategy = GetTopStrategy(epsp_target_strategies_);
  spdlog::get(LOGGER)->debug("AudioKi::GetEpspTargets: strategy: {}", epsp_target_strategy);
  // Target: activated neuron
  if (epsp_target_strategy == DESTROY_ACTIVATED_NEURONS && ignore_strategy != DESTROY_ACTIVATED_NEURONS) {
    // Find activated neurons on aproxiamte-way to destroy
    auto enemy_activated_neuerons = enemies_.front()->GetAllPositionsOfNeurons(ACTIVATEDNEURON);
    auto activated_neurons_on_way = GetAllActivatedNeuronsOnWay(enemy_activated_neuerons, way);
    // Take closest activated-neuron.
    activated_neurons_on_way = SortPositionsByDistance(nucleus_pos_, activated_neurons_on_way);
    if (activated_neurons_on_way.size() == 0)
      return GetEpspTargets(synapse_pos, way, DESTROY_ACTIVATED_NEURONS);
    return activated_neurons_on_way;
  }
  // Target: synapse
  else if (epsp_target_strategy == DESTROY_SYNAPSES && ignore_strategy != DESTROY_SYNAPSES) {
    auto enemy_synapses = GetEnemyNeuronsSortedByLeastDef(synapse_pos, UnitsTech::SYNAPSE);
    if (enemy_synapses.size() == 0)
      return GetEpspTargets(synapse_pos, way, DESTROY_SYNAPSES);
    return enemy_synapses;
  }
  // Target: resources
  else if (epsp_target_strategy == DESTROY_RESOURCES && ignore_strategy != DESTROY_RESOURCES) {
    auto enemy_resources = GetEnemyNeuronsSortedByLeastDef(synapse_pos, UnitsTech::RESOURCENEURON);
    if (enemy_resources.size() == 0)
      return GetEpspTargets(synapse_pos, way, DESTROY_RESOURCES);
    return enemy_resources;
  }
  // Target: nucleus 
  else {
    auto target = enemies_.front()->GetPositionOfClosestNeuron(synapse_pos, UnitsTech::NUCLEUS);
    return {target};
  }
}

std::vector<AudioKi::launch_target_pair_t> AudioKi::GetIpspLaunchesAndTargets(const std::list<position_t>& way, 
    const std::vector<position_t>& synapses, size_t ignore_strategy) const {
  size_t ipsp_target_strategy = GetTopStrategy(ipsp_target_strategies_);
  spdlog::get(LOGGER)->debug("AudioKi::GetIpspTargets: strategy: {}", ipsp_target_strategy);
  // Get ipsp-launch-synapses
  std::vector<position_t> ipsp_launch_synapes = AvailibleIpspLaunches(synapses, 4);
  std::vector<launch_target_pair_t> ipsp_launch_target_pair;
  // Target: activated neurons
  if (ipsp_target_strategy == BLOCK_ACTIVATED_NEURON && ignore_strategy != BLOCK_ACTIVATED_NEURON) {
    auto enemy_activated_neuerons = enemies_.front()->GetAllPositionsOfNeurons(ACTIVATEDNEURON);
    auto activated_neurons_on_way = GetAllActivatedNeuronsOnWay(enemy_activated_neuerons, way);
    activated_neurons_on_way = SortPositionsByDistance(nucleus_pos_, activated_neurons_on_way);
    for (size_t i=0; i<ipsp_launch_synapes.size() && i<activated_neurons_on_way.size(); i++)
      ipsp_launch_target_pair.push_back({ipsp_launch_synapes[i], activated_neurons_on_way[i]});
  }
  // Target: synapses
  else if (ipsp_target_strategy == BLOCK_SYNAPSES && ignore_strategy != BLOCK_SYNAPSES) {
    auto enemy_synapses = GetEnemyNeuronsSortedByLeastDef(nucleus_pos_, UnitsTech::SYNAPSE);
    if (enemy_synapses.size() == 0)
      return GetIpspLaunchesAndTargets(way, synapses, BLOCK_SYNAPSES);
    for (size_t i=0; i<ipsp_launch_synapes.size() && i<enemy_synapses.size(); i++)
      ipsp_launch_target_pair.push_back({ipsp_launch_synapes[i], enemy_synapses[i]});
  }
  // Target: resource neurons
  else if (ipsp_target_strategy == BLOCK_RESOURCES && ignore_strategy != BLOCK_RESOURCES) {
    auto enemy_resource_neurons = GetEnemyNeuronsSortedByLeastDef(nucleus_pos_, UnitsTech::RESOURCENEURON);
    if (enemy_resource_neurons.size() == 0)
      return GetIpspLaunchesAndTargets(way, synapses, BLOCK_RESOURCES);
    for (size_t i=0; i<ipsp_launch_synapes.size() && i<enemy_resource_neurons.size(); i++)
      ipsp_launch_target_pair.push_back({ipsp_launch_synapes[i], enemy_resource_neurons[i]});
  }
  // Otherwise: no target.
  return ipsp_launch_target_pair;
}

void AudioKi::CreateEpsps(position_t synapse_pos, position_t target_pos, int num_epsp_to_create, int speed_decrease) {
  spdlog::get(LOGGER)->debug("AudioKi::CreateEpsps: {}, speed_decrease: {}", num_epsp_to_create, speed_decrease);
  ChangeEpspTargetForSynapse(synapse_pos, target_pos);
  // Calculate update number of epsps to create and update interval
  for (int i=0; i<num_epsp_to_create && i<25; i++) {
    if (!AddPotential(synapse_pos, UnitsTech::EPSP, speed_decrease+i/2))
      break;
  }
}

void AudioKi::CreateIpsps(position_t synapse_pos, position_t target_pos, int num_ipsp_to_create) {
  spdlog::get(LOGGER)->debug("AudioKi::CreateIpsps: {}", num_ipsp_to_create);
  ChangeIpspTargetForSynapse(synapse_pos, target_pos);
  // Calculate update number of ipsps to create and update interval
  for (int i=0; i<num_ipsp_to_create && i<=9; i++) {
    if (!AddPotential(synapse_pos, UnitsTech::IPSP, i/2))
      break;
  }
}

void AudioKi::CreateSynapses() {
  spdlog::get(LOGGER)->debug("AudioKi::CreateSynapses.");
  size_t num_synapses = GetAllPositionsOfNeurons(SYNAPSE).size();
  // Don't add def, if curent activated neurons exeed num specified in building tactics.
  if (num_synapses >= building_tactics_[SYNAPSE])
    return;
  // Don't add def, out of concern for high resource bounds
  for (const auto& it : {OXYGEN, POTASSIUM}) {
    int diff_to_limit = resources_.at(it).limit()-(resources_.at(it).bound()+units_costs_.at(SYNAPSE).at(it));
    // Don't add if not enough of given resource, or to high bound.
    if (diff_to_limit < 25 || resources_.at(it).cur() < 25)
      return;
  }

  auto pos = field_->FindFree(nucleus_pos_, 1, 5);
  // If no more free positions are availible, try to extend range.
  if (pos.first == -1 && pos.second == -1)
    AddTechnology(UnitsTech::NUCLEUS_RANGE);
  // Otherwise build neuron.
  if (!AddNeuron(pos, UnitsTech::SYNAPSE, enemies_.front()->GetOneNucleus()))
    spdlog::get(LOGGER)->debug("AudioKi::CreateSynapses: not enough resources");
}

void AudioKi::CreateActivatedNeuron(bool force) {
  spdlog::get(LOGGER)->debug("AudioKi::CreateActivatedNeuron.");
  // Check whether activated-neurons can or should be built (scip if force).
  if (!force) {
    size_t num_activated_neurons = GetAllPositionsOfNeurons(ACTIVATEDNEURON).size();
    // Don't add def, if no synapses exists and atleast one def already exists.
    if (num_activated_neurons > 0 && GetAllPositionsOfNeurons(UnitsTech::SYNAPSE).size() == 0)
      return;
    // Don't add def, if curent activated neurons exeed num specified in building tactics.
    if (num_activated_neurons >= building_tactics_[ACTIVATEDNEURON])
      return;
    // Don't add def, out of concern for high resource bounds
    for (const auto& it : {OXYGEN, GLUTAMATE}) {
      int diff_to_limit = resources_.at(it).limit()-(resources_.at(it).bound()+units_costs_.at(ACTIVATEDNEURON).at(it));
      // Don't add if not enough of given resource, or to high bound.
      if (diff_to_limit < 25 || resources_.at(it).cur() < 25)
        return;
    }
  }

  // Find position to place neuron coresponding to tactics.
  position_t pos = {-1, -1};
  // Suround nucleus equally
  if (GetTopStrategy(activated_neuron_strategies_) == DEF_SURROUNG_FOCUS) {
    for (int i=1; i<cur_range_; i++) {
      auto positions = field_->GetAllInRange(nucleus_pos_, i, i-1, true);
      if (positions.size() > 0) {
        positions = SortPositionsByDistance(enemies_.front()->GetOneNucleus(), positions);
        pos = positions.front(); 
        break;
      }
    }
  }
  // Focus on front 
  else {
    auto way = field_->GetWay(nucleus_pos_, {enemies_.front()->GetOneNucleus()});
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
  // If no more free positions are availible, try to extend range.
  if (pos.first == -1 && pos.second == -1) {
    AddTechnology(UnitsTech::NUCLEUS_RANGE);
  }
  // Otherwise build neuron.
  else if (!AddNeuron(pos, UnitsTech::ACTIVATEDNEURON))
    spdlog::get(LOGGER)->debug("AudioKi::CreateActivatedNeuron: not enough resources");
}

void AudioKi::HandleIron() {
  int iron = resources_.at(IRON).cur();
  std::string resource_list;
  for (const auto& it : resource_tactics_)
    resource_list += resources_name_mapping.at(it) + ", ";
  if (iron == 0 || iron == 1) {
    if (LowIronResourceDistribution())
      return;
  }
  // Check if empty, no iron, or all resource already active
  if (resource_tactics_.empty() || iron == 0 || (resources_activated_.size() >= SEROTONIN && iron < 2))
    return;
  size_t resource = resource_tactics_.front();
  DistributeIron(resource);
  // If not active and iron left, distribute again.
  if (!resources_.at(resource).Active() && resources_.at(IRON).cur() > 0)
    DistributeIron(resource);
  // If resource is now activated, procceed to next resource and set resource as active.
  if (resources_.at(resource).Active()) {
    resource_tactics_.erase(resource_tactics_.begin());
    resources_activated_.insert(resource);
  }
}

bool AudioKi::LowIronResourceDistribution() {
  double max = 0;
  int resource = -1;
  bool resource_not_active = false;
  for (const auto& it : resources_) {
    // Most plentiful active resource
    if (it.second.Active() && it.second.cur() > max) {
      max = it.second.cur();
      resource = it.first;
    }
    if (!it.second.Active())
      resource_not_active = true;
  }
  // If there is a non-active resource, and a active resource with more than 40
  if (resource_not_active && resource != -1 && max > 40 && resources_.at(resource_tactics_.front()).cur() < 40) {
    RemoveIron(resource);
    RemoveIron(resource);
    DistributeIron(resource_tactics_.front());
    DistributeIron(resource_tactics_.front());
    resources_activated_.insert(resource_tactics_.front());
    resource_tactics_.erase(resource_tactics_.begin());
    resource_tactics_.insert(resource_tactics_.begin(), resource);
    resources_activated_.erase(resource);
    return true;
  }
  return false;
}

void AudioKi::NewTechnology() {
  // Check if empty.
  if (technology_tactics_.empty())
    return;
  // Research technology and remove from tech-list.
  size_t technology = technology_tactics_.front();
  if (!AddTechnology(technology))
    return;
  technology_tactics_.erase(technology_tactics_.begin());
  if (technology == SWARM) {
    for (auto& it : neurons_)
      if (it.second->type_ == SYNAPSE)
        it.second->set_swarm(true);
  }
  // If technology was already fully researched, research next technology right away.
  if (technologies_.at(technology).first == technologies_.at(technology).second)
    NewTechnology();
}

AudioKi::sorted_stragety AudioKi::SortStrategy(const std::map<size_t, size_t>& strategy) {
  std::list<std::pair<size_t, size_t>> sorted_strategy;
  for (const auto& it : strategy)
    sorted_strategy.push_back({it.second, it.first});
  sorted_strategy.sort();
  sorted_strategy.reverse();
  return sorted_strategy;
}

size_t AudioKi::GetTopStrategy(const std::map<size_t, size_t>& strategy) {
  size_t top_strategy = SortStrategy(strategy).front().second;
  return top_strategy;
}

size_t AudioKi::AvailibleIpsps() const {
  auto costs = units_costs_.at(UnitsTech::IPSP);
  float res = std::min(resources_.at(POTASSIUM).cur() / costs[POTASSIUM], 
      resources_.at(CHLORIDE).cur() / costs[CHLORIDE]);
  // If no ipsp-focus, reduce availible ipsps additionally
  if (GetTopStrategy(ipsp_epsp_strategies_) == EPSP_FOCUSED)
    res *= (float)ipsp_epsp_strategies_.at(IPSP_FOCUSED)/(float)ipsp_epsp_strategies_.at(EPSP_FOCUSED);
  return res*0.75;
}

size_t AudioKi::AvailibleEpsps(size_t ipsps_to_create) const {
  auto costs_ipsp = units_costs_.at(UnitsTech::IPSP);
  auto costs_epsp = units_costs_.at(UnitsTech::EPSP);
  // Function: `num_epsps_to_create = (cur_potassium-potassium_costs_ipsp)/potassium_costs_epsp`
  size_t res = (resources_.at(POTASSIUM).cur()-ipsps_to_create*costs_ipsp[POTASSIUM])/costs_epsp[POTASSIUM];
  return res;
}

std::vector<position_t> AudioKi::AvailibleIpspLaunches(const std::vector<position_t>& synapses, 
    int ipsp_per_synapse) const {
  size_t available_ipsps = AvailibleIpsps();  
  std::vector<position_t> result_positions;
  for (size_t i=0; i<available_ipsps; i+=ipsp_per_synapse)
    result_positions.push_back(synapses[i]);
  return result_positions;
}

std::vector<position_t> AudioKi::GetAllActivatedNeuronsOnWay(std::vector<position_t> activated_neurons, 
    const std::list<position_t>& way) {
  std::vector<position_t> result_positions;
  for (const auto& way_point : way) {
    for (auto it=activated_neurons.begin(); it!=activated_neurons.end();) {
      if (utils::Dist(way_point, *it) < 3) {
        result_positions.push_back(*it);
        activated_neurons.erase(it);
      }
      else 
        it++;
    }
  }
  return result_positions;
}

std::vector<position_t> AudioKi::SortPositionsByDistance(position_t start, const std::vector<position_t>& positions) {
  std::list<std::pair<size_t, position_t>> sorted_positions;
  for (const auto& it : positions)
    sorted_positions.push_back({utils::Dist(start, it), it});
  sorted_positions.sort();
  std::vector<position_t> result_positions;
  for (const auto& it : sorted_positions)
    result_positions.push_back(it.second);
  return result_positions; 
}

std::vector<position_t> AudioKi::GetEnemyNeuronsSortedByLeastDef(position_t start, int neuron_type) const {
  auto enemy_neurons = enemies_.front()->GetAllPositionsOfNeurons(neuron_type);
  std::list<std::pair<size_t, position_t>> sorted_positions;
  auto enemy_activated_neuerons = enemies_.front()->GetAllPositionsOfNeurons(ACTIVATEDNEURON);
  for (const auto& it : enemy_neurons) {
    auto way = field_->GetWay(start, {it});
    sorted_positions.push_back({GetAllActivatedNeuronsOnWay(enemy_activated_neuerons, way).size(), it});
  }
  sorted_positions.sort();
  sorted_positions.reverse();
  std::vector<position_t> result_positions;
  for (const auto& it : sorted_positions)
    result_positions.push_back(it.second);
  return result_positions; 
}

double AudioKi::GetMaxLevelExeedance() const {
  size_t cur_max = *std::max_element(last_level_peaks_.begin(), last_level_peaks_.end());
  return (double)cur_max/(audio_->analysed_data().max_level_-average_level_);
}

size_t AudioKi::NumberOfEpspsToCreate(const std::list<position_t>& way, size_t num_ipsps_to_create) const {
  size_t available_epsps = AvailibleEpsps(num_ipsps_to_create);
  size_t num_epsps_to_create = available_epsps*GetMaxLevelExeedance();
  // Always launch attack in first interval.
  if (cur_interval_.id_ == 0)
    return num_epsps_to_create;
  auto enemy_activated_neuerons = enemies_.front()->GetAllPositionsOfNeurons(ACTIVATEDNEURON);
  int activated_neuerons_on_way = GetAllActivatedNeuronsOnWay(enemy_activated_neuerons, way).size();
  if ((activated_neuerons_on_way-(int)num_ipsps_to_create) >= (int)num_epsps_to_create)
    return 0;
  return num_epsps_to_create;
}

int AudioKi::SynchAttacks(size_t epsp_way_length, size_t ipsp_way_length) const {
  int speed_boast = 50*technologies_.at(UnitsTech::ATK_POTENIAL).first;
  size_t ipsp_duration = ipsp_way_length*(IPSP_SPEED-speed_boast);
  size_t epsp_duration = epsp_way_length*(EPSP_SPEED-speed_boast);
  return (ipsp_duration < epsp_duration) ? 0 : (ipsp_duration-epsp_duration)+(IPSP_DURATION*0.75);
}

void AudioKi::Defend() {
  auto ps = enemies_.front()->GetEpspAtPosition();
  int enemy_potentials = std::accumulate(std::begin(ps), std::end(ps), 0, 
      [](int v, const std::map<position_t, int>::value_type& p) { return v+p.second; });
  spdlog::get(LOGGER)->debug("AudioKi::Defend. {} enemy potential coming.", enemy_potentials);
  if (enemy_potentials > 0) {
    size_t shortest_way = 20;
    // Get shortes enemy-way (==first enemy epsp to arrive) and ignore all way > 20
    std::list<position_t> way;
    for (const auto& it : enemies_.front()->potential()) {
      if (it.second._way.size() < shortest_way)
        way = it.second._way;
    }
    if (way.size() == 0) {
      spdlog::get(LOGGER)->debug("AudioKi::Defend. ommited since all enemies still far away.");
      return;
    }
    auto activated_neurons_on_way = GetAllActivatedNeuronsOnWay(GetAllPositionsOfNeurons(ACTIVATEDNEURON), way).size();
    int diff = enemy_potentials-(activated_neurons_on_way);
    // Get voltage of ai's nucleus closest to enemy target.
    int voltage = GetVoltageOfAttackedNucleus(way.back());
    if (diff > 0) {
      if (GetTopStrategy(def_strategies_) == DEF_IPSP_BLOCK) {
        if (!IpspDef(enemy_potentials, way, diff) && voltage+diff*2 > 9)
          CreateExtraActivatedNeurons(enemy_potentials, way, diff);
      }
      else {
        if (!CreateExtraActivatedNeurons(enemy_potentials, way, diff) && voltage+diff*2 > 9)
          IpspDef(enemy_potentials, way, diff);
      }
    }
    else {
      spdlog::get(LOGGER)->debug("AudioKi::Defend. ommited since all enemies can be defended: diff: {}", diff);
    }
  }
}

bool AudioKi::IpspDef(int enemy_potentials, std::list<position_t> way, int diff) {
  spdlog::get(LOGGER)->debug("AudioKi::IpspDef");
  // If ipsps cannot be build, ommit.
  if (GetMissingResources(IPSP).size() > 0) {
    spdlog::get(LOGGER)->debug("AudioKi::IpspDef. Ommited (negativly), since missing resources!");
    return false;
  }
  // If less than 2 waypoints, try to add waypoints be researching.
  if (technologies_.at(WAY).first < 2) {
    AddTechnology(WAY);
  }
  // Otherwise, build `diff` number of ipsps.
  auto sorted_synapses = SortPositionsByDistance(way.back(), GetAllPositionsOfNeurons(UnitsTech::SYNAPSE));
  if (sorted_synapses.size() > 0) {
    auto synapse_pos = sorted_synapses.front();
    int num_waypoints = technologies_.at(WAY).first;
    way.reverse();
    auto it = way.begin();
    for (int i=0; i<num_waypoints; i++) {
      AddWayPosForSynapse(synapse_pos, *it);
      std::advance(it, way.size()/(num_waypoints+1));
    }
    spdlog::get(LOGGER)->debug("AudioKi::IpspDef. Creating {} ipsps using {} waypoints", diff, num_waypoints);
    CreateIpsps(synapse_pos, way.back(), diff);
  }
  return true;
}

bool AudioKi::CreateExtraActivatedNeurons(int enemy_potentials, const std::list<position_t>& way, int diff) {
  spdlog::get(LOGGER)->debug("AudioKi::CreateExtraActivatedNeurons. creating {} activated neurons", diff);
  // Build activated neurons based on enemy epsp-attack launch.
  while (diff-- > 0) {
    if (GetMissingResources(ACTIVATEDNEURON).size() > 0) {
      spdlog::get(LOGGER)->debug("AudioKi::CreateExtraActivatedNeurons. Ommited, since missing resources!");
      return false;
    }
    CreateActivatedNeuron(true);
  }
  return true;
}

void AudioKi::HandleHighBound() {
  // Finid resouce with highest bound
  int highest_bound = 70;
  int resouce = -1;
  for (const auto& it : resources_) {
    if (it.second.bound() > highest_bound) {
      highest_bound = it.second.bound();
      resouce = it.first;
    }
    if (it.first == OXYGEN && it.second.bound() > 65) {
      resouce = OXYGEN;
      break;
    }
  }
  // If no resouce with high bound found, return.
  if (resouce == -1)
    return;
  spdlog::get(LOGGER)->info("AudioKi::HandleHighBound. Using fixes on {}", resources_name_mapping.at(resouce));
  // First, try to increase resouce bounds
  if (AddTechnology(TOTAL_RESOURCE)) {
    spdlog::get(LOGGER)->info("AudioKi::HandleHighBound. Successfully increased resouce bounds.");
    return;
  }
  // Second, try adding iron to resouce
  int iron = resources_.at(IRON).cur();
  if (iron > 2 && resources_activated_.size() >= SEROTONIN) {
    // Distribute all iron but the last to resource with highest bound
    for (int i=1; i<iron; i++) DistributeIron(resouce);
    spdlog::get(LOGGER)->info("AudioKi::HandleHighBound. Successfully distributed {} iron to resouce.", iron-1);
    return;
  }
  // Last option (only for oxygen and only if oxygen is low): destroy synapse
  if (resouce == OXYGEN && resources_.at(resouce).cur() < 10) {
    auto synapse_positions = GetAllPositionsOfNeurons(SYNAPSE);
    if (synapse_positions.size() > 2) {
      // Make sure way does not lead through enemy teritories, by setting first
      // way-point to target.
      AddWayPosForSynapse(synapse_positions.front(), synapse_positions.back(), true);
      CreateEpsps(synapse_positions.front(), synapse_positions.back(), 3, 0);
    }
    spdlog::get(LOGGER)->info("AudioKi::HandleHighBound. Tring to destroy synapse...");
  }
}

int AudioKi::GetVoltageOfAttackedNucleus(position_t enemy_target_pos) {
  auto sorted_nucleus = SortPositionsByDistance(enemy_target_pos, GetAllPositionsOfNeurons(NUCLEUS));
  if (sorted_nucleus.size() > 0) {
    try {
      return neurons_.at(sorted_nucleus.front())->voltage();
    } catch (std::exception& e) {
      spdlog::get(LOGGER)->warn("AudioKi::GetVoltageOfAttackedNucleus: Accessed nucleus but didn't exist.");
      return -1;
    }
  }
  spdlog::get(LOGGER)->warn("AudioKi::GetVoltageOfAttackedNucleus: No nucleus found");
  return -1;
}

std::vector<position_t> AudioKi::FindBestWayPoints(position_t synapse_pos, position_t target_pos) {
  std::vector<position_t> waypoints;
  // Get all enemy activated neurons.
  auto activated_neurons = enemies_.front()->GetAllPositionsOfNeurons(ACTIVATEDNEURON);
  size_t cur_ans_on_way = GetAllActivatedNeuronsOnWay(activated_neurons, 
          field_->GetWay(synapse_pos, {target_pos})).size();

  // Find best waypoint for each possible way-point to set (based on technology WAY)
  for (size_t i=0; i<technologies_.at(WAY).first; i++) {
    // Get all possible ways with distance 8-9, 7-8, 6-7 (circle slowly draws closer...)
    std::vector<position_t> possible_way_points = field_->GetAllInRange(target_pos, 9-1, 8-i, true);
    // Find way with least activated neurons on way.
    position_t waypoint = DEFAULT_POS;
    for (const auto& pos : possible_way_points) {
      // Add this possible waypoint position and target to waypoints.
      waypoints.insert(waypoints.end(), {pos, target_pos});
      size_t num_ans_on_way = GetAllActivatedNeuronsOnWay(activated_neurons, 
          field_->GetWay(synapse_pos, waypoints)).size();
      if (num_ans_on_way < cur_ans_on_way) {
        cur_ans_on_way = num_ans_on_way;
        waypoint = pos;
      }
      // Remove target and waypoint again.
      waypoints.pop_back();
      waypoints.pop_back();
    }
    // Add final waypoint.
    if (waypoint != DEFAULT_POS)
      waypoints.push_back(waypoint);
    spdlog::get(LOGGER)->debug("AudioKi::FindBestWayPoints. Added waypoint: {}", utils::PositionToString(waypoint));
  }
  return waypoints;
}
