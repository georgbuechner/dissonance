#ifndef SRC_OBJECT_TRANSFER_H_
#define SRC_OBJECT_TRANSFER_H_
#include <string>
#include <vector>

#include "nlohmann/json.hpp"
#include "share/defines.h"
#include "spdlog/spdlog.h"

#include "share/objects/units.h"
#include "share/tools/utils/utils.h"

#define LOGGER "logger"
#define COLOR_DEFAULT 3

class Transfer {
  public:
    // Objects
    struct Symbol {
      std::string symbol_;
      int color_;
      std::string str() { 
        return "Symbol: " + symbol_ + "(color: " + std::to_string(color_) + ")";
      }
    };

    struct Resource {
      std::string value_;
      std::string bound_;
      std::string limit_;
      std::string iron_;
      bool active_;
    };

    struct Technology {
      std::string cur_;
      std::string max_;
      bool active_;
    };

    Transfer() {initialized_ = false;};
    Transfer(nlohmann::json json) {
      spdlog::get(LOGGER)->info("from json: players {}", json["players"].dump());
      // build players from json.
      players_ = json["players"].get<std::map<std::string, std::string>>();

      // build field from json
      spdlog::get(LOGGER)->info("from json: field {}", json["f"].dump());
      for (const auto& it : json["f"].get<std::vector<nlohmann::json>>()) {
        std::vector<Symbol> vec;
        for (const auto& jt : it.get<std::vector<nlohmann::json>>())
          vec.push_back({jt["s"].get<std::string>(), jt["c"].get<int>()});
        field_.push_back(vec);
      }

      // build resources from json.
      spdlog::get(LOGGER)->info("from json: resources {}", json["r"].dump());
      if (json.contains("r")) {
        for (const auto& it : json["r"].get<std::map<std::string, nlohmann::json>>())
          resources_[stoi(it.first)] = {it.second["v"], it.second["b"], it.second["l"], it.second["i"], it.second["a"]};
      }

      // build technologies from json.
      spdlog::get(LOGGER)->info("from json: technologies {}", json["t"].dump());
      if (json.contains("t")) {
        for (const auto& it : json["t"].get<std::map<std::string, nlohmann::json>>())
          technologies_[stoi(it.first)] = {it.second["c"], it.second["m"], it.second["a"]};
      }

      // build potentials from string 
      spdlog::get(LOGGER)->info("from json: potentials {}", json["p"].dump());
      if (json.contains("p")) {
        for (const auto& it : json["p"].get<std::map<std::string, nlohmann::json>>())
          potentials_[utils::PositionFromString(it.first)] = {it.second["s"].get<std::string>(), it.second["c"]};
      }
      
      // build new-dead-neurons from string 
      spdlog::get(LOGGER)->info("from json: new dead neurons {}", json["d"].dump());
      if (json.contains("d")) {
        for (const auto& it : json["d"].get<std::map<std::string, int>>())
          new_dead_neurons_[utils::PositionFromString(it.first)] = it.second;
      }

      // Audio Played
      if (json.contains("a"))
        audio_played_ = json["a"];

      initialized_ = true;
      spdlog::get(LOGGER)->info("DONE");
    }

    // getter
    bool initialized() {
      return initialized_;
    }

    std::vector<std::vector<Symbol>> field() const {
      return field_;
    };

    std::map<std::string, std::string> players() const {
      return players_;
    }

    std::string PlayersToString() const {
      std::string str;
      for (const auto& it : players_)
        str += it.first + ": " + it.second + " | ";
      str.erase(str.length()-3);
      return str;
    }

    std::map<int, Resource> resources() const {
      return resources_;
    }
    std::map<int, Technology> technologies() const {
      return technologies_;
    }
    std::map<position_t, int> new_dead_neurons() const {
      return new_dead_neurons_;
    }

    std::map<position_t, std::pair<std::string, int>> potentials() {
      return potentials_;
    }
    float audio_played() const {
      return audio_played_;
    }

    // setter
    void set_field(std::vector<std::vector<Symbol>> field) {
      field_ = field;
    }
    void set_players(std::map<std::string, std::string> players) {
      players_ = players;
    }
    void set_resources(std::map<int, Resource> resources) {
      resources_ = resources;
    }
    void set_technologies(std::map<int, Technology> technologies) {
      technologies_ = technologies;
    }
    void set_new_dead_neurons(std::map<position_t, int> new_dead_neurons) {
      new_dead_neurons_ = new_dead_neurons;
    }

    void set_potentials(std::map<position_t, std::pair<std::string, int>> potentials) {
      potentials_ = potentials;
    }
    void set_audio_played(float audio_played) {
      audio_played_ = (audio_played < 0) ? 0 : audio_played;
    }

    nlohmann::json json() {
      nlohmann::json data; 
      data["players"] = players_;
      data["f"] = nlohmann::json::array();
      for (const auto& it : field_) {
        nlohmann::json vec = nlohmann::json::array();
        for (const auto& jt : it)
          vec.push_back({{"s", jt.symbol_}, {"c", jt.color_}});
        data["f"].push_back(vec);
      }
      data["r"] = nlohmann::json::object();
      for (const auto& it : resources_) {
        data["r"][std::to_string(it.first)] = {{"v", it.second.value_}, {"b", it.second.bound_}, 
          {"l", it.second.limit_}, {"i", it.second.iron_}, {"a", it.second.active_}};
      }
      data["t"] = nlohmann::json::object();
      for (const auto& it : technologies_) {
        data["t"][std::to_string(it.first)] = {{"c", it.second.cur_}, {"m", it.second.max_}, {"a", it.second.active_}};
      }
      data["d"] = nlohmann::json::object();
      for (const auto& it : new_dead_neurons_) 
        data["d"][utils::PositionToString(it.first)] = it.second;
      data["a"] = audio_played_;
      data["p"] = nlohmann::json::object();
      for (const auto& it : potentials_) {
        data["p"][utils::PositionToString(it.first)] = {{"s", it.second.first}, {"c", it.second.second}};
      }
      return data;
    }

  private:
    std::vector<std::vector<Symbol>> field_;
    std::map<position_t, std::pair<std::string, int>> potentials_;
    std::map<std::string, std::string> players_;
    std::map<int, Resource> resources_;
    std::map<int, Technology> technologies_;
    std::map<position_t, int> new_dead_neurons_;
    float audio_played_;
    bool initialized_;
};

#endif
