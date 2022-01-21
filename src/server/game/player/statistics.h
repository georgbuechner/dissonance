#ifndef SRC_SERVER_GAMES_PLAYER_STATISTICS_H_
#define SRC_SERVER_GAMES_PLAYER_STATISTICS_H_

#include "share/constants/codes.h"
#include <iostream>
#include <map>
#include <string>

class Statictics {
  public:
    Statictics() : epsp_swallowed_(0) {}

    // getter
    
    // setter


    // methods
    void AddNewNeuron(int unit) {
      neurons_build_[unit]++;
    }
    void AddNewPotential(int unit) {
      potentials_build_[unit]++;
    }
    void AddKillderPotential(std::string id) {
      int unit = (id.find("ipsp") != std::string::npos) ? IPSP : EPSP;
      potentials_killed_[unit]++;
    }
    void AddLostPotential(std::string id) {
      int unit = (id.find("ipsp") != std::string::npos) ? IPSP : EPSP;
      potentials_lost_[unit]++;
    }
    void AddEpspSwallowed() {
      epsp_swallowed_++;
    }

    void print() {
      std::cout << "Neurons built: " << std::endl;
      for (const auto& it : neurons_build_) 
        std::cout << " - " << units_tech_name_mapping.at(it.first) << ": " << it.second << std::endl;
      std::cout << "Potentials built: " << std::endl;
      for (const auto& it : potentials_build_) 
        std::cout << " - " << units_tech_name_mapping.at(it.first) << ": " << it.second << std::endl;
      std::cout << "Potentials killed: " << std::endl;
      for (const auto& it : potentials_killed_) 
        std::cout << " - " << units_tech_name_mapping.at(it.first) << ": " << it.second << std::endl;
      std::cout << "Potentials lost: " << std::endl;
      for (const auto& it : potentials_lost_) 
        std::cout << " - " << units_tech_name_mapping.at(it.first) << ": " << it.second << std::endl;
      std::cout << "Enemy epsps swallowed by ipsp: " << epsp_swallowed_ << std::endl;
    }

  private:
    std::map<int, int> neurons_build_;
    std::map<int, int> potentials_build_;
    std::map<int, int> potentials_killed_;
    std::map<int, int> potentials_lost_;
    int epsp_swallowed_;
};

#endif
