#ifndef SRC_KI_H_H
#define SRC_KI_H_H

#include <algorithm>
#include <chrono>
#include <iterator>
#include <list>
#include <mutex>
#include <shared_mutex>
#include <vector>
#include <random>
#include <spdlog/spdlog.h>

#include "constants/codes.h"
#include "game/field.h"
#include "player/player.h"
#include "objects/units.h"
#include "utils/utils.h"

#define LOGGER "logger"

#define MIN_POTENTIAL_FREQUENCY 50
#define MAX_ACTIVATED_NEURONS 8
#define MIN_RESOURCE_CURVE 1

#define POTENTIAL_FREQUENCY_UPDATE 20
#define ACTIVATED_NEURONS_UPDATE 2
#define RESOURCE_CURVE_UPDATE 1

class Ki {
  public:
    Ki(Player* player_one, Player* ki) {
      ki_ = ki;
      player_one_ = player_one;
       
      new_potential_frequency_ = 150;
      update_frequency_ = 20000;
      max_activated_neurons_ = 3;

      attacks_ = {10, 10, 10, 10, 20, 10, 40, 30, 40, 50};
      technologies_.insert(technologies_.end(), UnitsTech::CURVE, 2);
      technologies_.insert(technologies_.end(), UnitsTech::TOTAL_RESOURCE, 2);
      technologies_.insert(technologies_.end(), UnitsTech::ATK_POTENIAL, 3);
      technologies_.insert(technologies_.end(), UnitsTech::ATK_SPEED, 3);
      technologies_.insert(technologies_.end(), UnitsTech::ATK_DURATION, 3);
      technologies_.insert(technologies_.end(), UnitsTech::DEF_POTENTIAL, 3);
      technologies_.insert(technologies_.end(), UnitsTech::DEF_SPEED, 3);
      auto rng = std::default_random_engine {};
      std::shuffle(std::begin(technologies_), std::begin(technologies_), rng);

      last_potential_ = std::chrono::steady_clock::now(); 
      last_update_ = std::chrono::steady_clock::now(); 
    }

    void SetUpKi(int difficulty) {
      // Distribute iron to oxygen and glutamate.
      ki_->DistributeIron(Resources::OXYGEN);
      ki_->DistributeIron(Resources::GLUTAMATE);

      // Increase update-frequency depending on difficulty.
      update_frequency_ -= 1000*(difficulty-1);
    }
    
    // methods
    void UpdateKi(Field* field) {
      if (ki_->GetMissingResources(UnitsTech::SYNAPSE).size() == 0)
        CreateSynapses(field);
      if (ki_->resources().at(Resources::POTASSIUM).first > attacks_.front())
        CreateEpsp(field);
      if (ki_->GetMissingResources(UnitsTech::ACTIVATEDNEURON).size() == 0)
        CreateActivatedNeuron(field);
      if (ki_->iron() > 0)
        DistributeIron(field);
      if (utils::get_elapsed(last_update_, std::chrono::steady_clock::now()) > update_frequency_)
        Update();
    }

  protected:
    Player* player_one_;
    Player* ki_;

    int new_potential_frequency_;  ///< determins how fast potential is added once resources are reached.
    int update_frequency_; 
    std::list<int> attacks_;  ///< determins how many resources to gather before launching an attack.
    std::vector<int> technologies_;  ///< determins how many resources to gather before launching an attack.
    unsigned int max_activated_neurons_;  ///< determins maximum number of towers

    std::chrono::time_point<std::chrono::steady_clock> last_potential_;
    std::chrono::time_point<std::chrono::steady_clock> last_update_;

    // methods
    
    void CreateSynapses(Field* field) {
      spdlog::get(LOGGER)->debug("Ki::CreateSynapses");
      if (ki_->GetAllPositionsOfNeurons(UnitsTech::SYNAPSE).size() == 0) {
        spdlog::get(LOGGER)->debug("Ki::CreateSynapses: createing synapses.");
        auto pos = field->FindFree(ki_->nucleus_pos().first, ki_->nucleus_pos().second, 1, 5);
        ki_->AddNeuron(pos, UnitsTech::SYNAPSE, player_one_->nucleus_pos());
        field->AddNewUnitToPos(pos, UnitsTech::SYNAPSE);
        spdlog::get(LOGGER)->debug("Ki::CreateSynapses: done.");
      }
    }

    void CreateEpsp(Field* field) {
      spdlog::get(LOGGER)->debug("Ki::CreateEpsp");
      // Check that atleast one synapses exists.
      if (ki_->GetAllPositionsOfNeurons(UnitsTech::SYNAPSE).size() > 0) {
        spdlog::get(LOGGER)->debug("Ki::CreateEpsp: get closes position");
        auto synapse_pos = ki_->GetAllPositionsOfNeurons(UnitsTech::SYNAPSE).front();
        // Get neares nucleus_pos
        int min_dist = 999;
        Position target_pos = {-1, -1}; 
        for (auto it : player_one_->GetAllPositionsOfNeurons(UnitsTech::NUCLEUS)) {
          int dist = utils::dist(synapse_pos, it);
          if(dist < min_dist) {
            target_pos = it;
            min_dist = dist;
          }
        }
        ki_->ChangeEpspTargetForSynapse(synapse_pos, target_pos);
        // Create epsps:
        spdlog::get(LOGGER)->debug("Ki::CreateEpsp: create epsps.");
        while (ki_->GetMissingResources(UnitsTech::EPSP).size() == 0) {
          auto cur_time_b = std::chrono::steady_clock::now(); 
          if (utils::get_elapsed(last_potential_, cur_time_b) > new_potential_frequency_) 
            last_potential_ = std::chrono::steady_clock::now();
          else 
            continue;
          spdlog::get(LOGGER)->debug("Ki::CreateEpsp: create one epsp.");
          ki_->AddPotential(synapse_pos, field, UnitsTech::EPSP);
        }
        spdlog::get(LOGGER)->debug("Ki::CreateEpsp: done.");
        if (attacks_.size() > 1)
          attacks_.pop_front();
      }
    }

    void CreateActivatedNeuron(Field* field) {
      spdlog::get(LOGGER)->debug("Ki::CreateActivatedNeuron.");
      if (max_activated_neurons_ >= ki_->GetNumActivatedNeurons()) {
        spdlog::get(LOGGER)->debug("Ki::CreateActivatedNeuron: checking resources");
        // Only add def, if a barrak already exists, or no def exists.
        if (!(ki_->GetNumActivatedNeurons() > 0 
            && ki_->GetAllPositionsOfNeurons(UnitsTech::SYNAPSE).size() == 0)
            || ki_->resources().at(Resources::OXYGEN).first > 40) {
          spdlog::get(LOGGER)->debug("Ki::CreateActivatedNeuron: createing neuron");
          auto pos = field->FindFree(ki_->nucleus_pos().first, ki_->nucleus_pos().second, 1, 5);
          field->AddNewUnitToPos(pos, UnitsTech::ACTIVATEDNEURON);
          ki_->AddNeuron(pos, UnitsTech::ACTIVATEDNEURON);
          spdlog::get(LOGGER)->debug("Ki::CreateActivatedNeuron: done");
        }
      }
    }

    void DistributeIron(Field* field) {
      if (ki_->oxygen_boast() > ki_->max_oxygen()-5 
          && ki_->technologies().at(UnitsTech::TOTAL_OXYGEN).first 
          >= ki_->technologies().at(UnitsTech::TOTAL_OXYGEN).second) {
        spdlog::get(LOGGER)->debug("Ki::DistributeIron: increases total oxygen");
        ki_->AddTechnology(UnitsTech::TOTAL_OXYGEN);
      }
      else if (ki_->IsActivatedResource(Resources::POTASSIUM)) {
        spdlog::get(LOGGER)->debug("Ki::DistributeIron: increases oxygen boast");
        ki_->DistributeIron(Resources::OXYGEN);
      }
      else if (ki_->iron() == 2) {
        spdlog::get(LOGGER)->debug("Ki::DistributeIron: add receiving potassium");
        ki_->DistributeIron(Resources::POTASSIUM);
      }
      if (ki_->oxygen_boast() > 5) {
        spdlog::get(LOGGER)->debug("Ki::DistributeIron: getting technologies.");
        ki_->AddTechnology(technologies_.back());
        technologies_.pop_back();
      }
    }

    void Update() {
      spdlog::get(LOGGER)->debug("Ki::Update stuff.");
      // Update new-potential-frequency
      if (new_potential_frequency_-POTENTIAL_FREQUENCY_UPDATE > MIN_POTENTIAL_FREQUENCY)
        new_potential_frequency_ -= POTENTIAL_FREQUENCY_UPDATE;

      // Update max number of activated_neurons 
      if (max_activated_neurons_+ACTIVATED_NEURONS_UPDATE < MAX_ACTIVATED_NEURONS)
        max_activated_neurons_ += ACTIVATED_NEURONS_UPDATE;

      // Update resource_curve
      if (ki_->resource_curve()-RESOURCE_CURVE_UPDATE > MIN_RESOURCE_CURVE)
        ki_->set_resource_curve(ki_->resource_curve()-RESOURCE_CURVE_UPDATE);
      spdlog::get(LOGGER)->debug("Ki::Done.");
      last_update_ = std::chrono::steady_clock::now(); 
    }
};

#endif
