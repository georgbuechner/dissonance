#ifndef SRC_SERVER_GAMES_PLAYER_STATISTICS_H_
#define SRC_SERVER_GAMES_PLAYER_STATISTICS_H_

#include "nlohmann/json.hpp"
#include "share/constants/codes.h"
#include <iostream>
#include <map>
#include <string>

class Statictics {
  public:
    Statictics() : epsp_swallowed_(0) {}
    Statictics(nlohmann::json json) {
      player_color_ = json["player_color"].get<int>();
      neurons_build_ = json["neurons_build"].get<std::map<int, int>>();
      potentials_build_ = json["potentials_build"].get<std::map<int, int>>();
      potentials_killed_ = json["potentials_killed"].get<std::map<int, int>>();
      potentials_lost_ = json["potentials_lost"].get<std::map<int, int>>();
      epsp_swallowed_ = json["epsp_swallowed"].get<int>();
      resources_ = json["resources"].get<std::map<int, std::map<std::string, double>>>();
    }

    // getter
    std::map<int, std::map<std::string, double>>& get_resources() { return resources_; }
    std::map<int, std::map<std::string, double>> resources() const { return resources_; }
    int player_color() const { return player_color_; }
    std::map<int, int> neurons_build() const { return neurons_build_; }
    std::map<int, int> potentials_build() const { return potentials_build_; }
    std::map<int, int> potentials_killed() const { return potentials_killed_; }
    std::map<int, int> potentials_lost() const { return potentials_lost_; }
    int epsp_swallowed() const { return epsp_swallowed_; }

    
    // setter
    void set_color(int color) { player_color_ = color; }

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

    nlohmann::json ToJson() {
      nlohmann::json json;
      json["player_color"] = player_color_;
      json["resources"] = resources_;
      json["neurons_build"] = neurons_build_;
      json["potentials_build"] = potentials_build_;
      json["potentials_killed"] = potentials_killed_;
      json["potentials_lost"] = potentials_lost_;
      json["epsp_swallowed"] = epsp_swallowed_;
      return json;
    }

  private:
    int player_color_;
    std::map<int, int> neurons_build_;
    std::map<int, int> potentials_build_;
    std::map<int, int> potentials_killed_;
    std::map<int, int> potentials_lost_;
    int epsp_swallowed_;
    std::map<int, std::map<std::string, double>> resources_;
};

#endif
