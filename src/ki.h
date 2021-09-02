#ifndef SRC_KI_H_H
#define SRC_KI_H_H

#include "codes.h"
#include "field.h"
#include "player.h"
#include "units.h"
#include "utils.h"
#include <algorithm>
#include <chrono>
#include <iterator>
#include <list>
#include <mutex>
#include <shared_mutex>
#include <vector>
#include <random>

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

      // Add extra stating resources to ki.
      for (int i=0; i<2*difficulty; i++) 
        ki_->IncreaseResources();

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

  private:
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
      if (ki_->synapses().size() == 0) {
        auto pos = field->FindFree(ki_->nucleus_pos().first, ki_->nucleus_pos().second, 1, 5);
        ki_->AddNeuron(pos, UnitsTech::SYNAPSE, player_one_->nucleus_pos());
        field->AddNewUnitToPos(pos, UnitsTech::SYNAPSE);
      }
    }

    void CreateEpsp(Field* field) {
      // Check that atleast one synapses exists.
      if (ki_->synapses().size() > 0) {
        auto synapse_pos = ki_->synapses().begin()->first;
        int min_dist = 999;
        Position target_pos = {-1, -1}; 
        for (const auto &it : player_one_->all_nucleus()) {
          int dist = utils::dist(synapse_pos, it.first);
          if(dist < min_dist) {
            target_pos = it.first;
            min_dist = dist;
          }
        }
        ki_->ChangeEpspTargetForSynapse(synapse_pos, target_pos);
        while (ki_->GetMissingResources(UnitsTech::EPSP).size() == 0) {
          auto cur_time_b = std::chrono::steady_clock::now(); 
          if (utils::get_elapsed(last_potential_, cur_time_b) > new_potential_frequency_) 
            last_potential_ = std::chrono::steady_clock::now();
          else 
            continue;
          ki_->AddPotential(synapse_pos, field, UnitsTech::EPSP);
        }
        if (attacks_.size() > 1)
          attacks_.pop_front();
      }
    }

    void CreateActivatedNeuron(Field* field) {
      if (max_activated_neurons_ >= ki_->activated_neurons().size()) {
        // Only add def, if a barrak already exists, or no def exists.
        if (!(ki_->activated_neurons().size() > 0 && ki_->synapses().size() == 0)
            || ki_->resources().at(Resources::OXYGEN).first > 40) {
          auto pos = field->FindFree(ki_->nucleus_pos().first, ki_->nucleus_pos().second, 1, 5);
          field->AddNewUnitToPos(pos, UnitsTech::ACTIVATEDNEURON);
          ki_->AddNeuron(pos, UnitsTech::ACTIVATEDNEURON);
        }
      }
    }

    void DistributeIron(Field* field) {
      if (ki_->oxygen_boast() > ki_->max_oxygen()-5 
          && ki_->technologies().at(UnitsTech::TOTAL_OXYGEN).first 
          >= ki_->technologies().at(UnitsTech::TOTAL_OXYGEN).second)
        ki_->AddTechnology(UnitsTech::TOTAL_OXYGEN);
      else if (ki_->IsActivatedResource(Resources::POTASSIUM))
        ki_->DistributeIron(Resources::OXYGEN);
      else if (ki_->iron() == 2)
        ki_->DistributeIron(Resources::POTASSIUM);
      if (ki_->oxygen_boast() > 5) {
        ki_->AddTechnology(technologies_.back());
        technologies_.pop_back();
      }
    }

    void Update() {
      // Update new-potential-frequency
      if (new_potential_frequency_-POTENTIAL_FREQUENCY_UPDATE > MIN_POTENTIAL_FREQUENCY)
        new_potential_frequency_ -= POTENTIAL_FREQUENCY_UPDATE;

      // Update max number of activated_neurons 
      if (max_activated_neurons_+ACTIVATED_NEURONS_UPDATE < MAX_ACTIVATED_NEURONS)
        max_activated_neurons_ += ACTIVATED_NEURONS_UPDATE;

      // Update resource_curve
      if (ki_->resource_curve()-RESOURCE_CURVE_UPDATE > MIN_RESOURCE_CURVE)
        ki_->set_resource_curve(ki_->resource_curve()-RESOURCE_CURVE_UPDATE);
    }
};

#endif
