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

    Transfer() {initialized_ = false; macro_ = -1;};
    Transfer(nlohmann::json json) {
      spdlog::get(LOGGER)->debug("from json: players {}", json["players"].dump());
      // build players from json.
      players_ = json["players"].get<std::map<std::string, std::pair<std::string, int>>>();

      // build field from json
      spdlog::get(LOGGER)->debug("from json: field {}", json["f"].dump());
      for (const auto& it : json["f"].get<std::vector<nlohmann::json>>()) {
        std::vector<Symbol> vec;
        for (const auto& jt : it.get<std::vector<nlohmann::json>>())
          vec.push_back({jt["s"].get<std::string>(), jt["c"].get<int>()});
        field_.push_back(vec);
      }

      macro_ = json.value("m", -1);

      // build graph_positions
      if (json.contains("g"))
        graph_positions_ = json["g"].get<std::vector<position_t>>();

      // build resources from json.
      spdlog::get(LOGGER)->debug("from json: resources {}", json["r"].dump());
      if (json.contains("r")) {
        for (const auto& it : json["r"].get<std::map<std::string, nlohmann::json>>())
          resources_[stoi(it.first)] = {it.second["v"], it.second["b"], it.second["l"], it.second["i"], it.second["a"]};
      }

      // build technologies from json.
      spdlog::get(LOGGER)->debug("from json: technologies {}", json["t"].dump());
      if (json.contains("t")) {
        for (const auto& it : json["t"].get<std::map<std::string, nlohmann::json>>())
          technologies_[stoi(it.first)] = {it.second["c"], it.second["m"], it.second["a"]};
      }

      // build potentials from string 
      spdlog::get(LOGGER)->debug("from json: potentials {}", json["p"].dump());
      if (json.contains("p")) {
        for (const auto& it : json["p"].get<std::map<std::string, nlohmann::json>>())
          potentials_[utils::PositionFromString(it.first)] = {it.second["s"].get<std::string>(), it.second["c"]};
      }
      
      // build new-dead-neurons from string 
      spdlog::get(LOGGER)->debug("from json: new dead neurons {}", json["d"].dump());
      if (json.contains("d")) {
        for (const auto& it : json["d"].get<std::map<std::string, int>>())
          new_dead_neurons_[utils::PositionFromString(it.first)] = it.second;
      }

      // build build-options.
      if (json.contains("b"))
        build_options_ = json["b"].get<std::vector<bool>>();
      // build build-options.
      if (json.contains("s"))
        synapse_options_ = json["s"].get<std::vector<bool>>();


      // Audio Played
      if (json.contains("a"))
        audio_played_ = json["a"];

      initialized_ = true;
      spdlog::get(LOGGER)->debug("DONE");
    }

    // getter
    bool initialized() {
      return initialized_;
    }
    std::vector<std::vector<Symbol>> field() const {
      return field_;
    };
    std::map<std::string, std::pair<std::string, int>> players() const {
      return players_;
    }
    std::vector<position_t> graph_positions() const {
      return graph_positions_;
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
    std::map<position_t, std::pair<std::string, int>> potentials() const {
      return potentials_;
    }
    std::vector<bool> build_options() const {
      return build_options_;
    }
    std::vector<bool> synapse_options() const {
      return synapse_options_;
    }
    float audio_played() const {
      return audio_played_;
    }
    int macro() const {
      return macro_;
    }

    // setter
    void set_field(std::vector<std::vector<Symbol>> field) {
      field_ = field;
    }
    void set_graph_positions(std::vector<position_t> graph_positions) {
      graph_positions_ = graph_positions;
    }
    void set_players(std::map<std::string, std::pair<std::string, int>> players) {
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
    void set_build_options(std::vector<bool> build_options) {
      build_options_ = build_options;
    }
    void set_synapse_options(std::vector<bool> synapse_options) {
      synapse_options_ = synapse_options;
    }
    void set_audio_played(float audio_played) {
      audio_played_ = (audio_played < 0) ? 0 : audio_played;
    }
    void set_macro(int macro) { 
      macro_ = macro; 
    }

    // methods: 
    
    t_topline PlayersToPrint() const {
      t_topline print;
      for (const auto& it : players_) {
        print.push_back({it.first + ": " + it.second.first, it.second.second});
        print.push_back({" | ", COLOR_DEFAULT});
      }
      if (print.size() > 0)
        print.pop_back();
      return print;
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
      data["g"] = graph_positions_;
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
      data["b"] = build_options_;
      data["s"] = synapse_options_;
      data["m"] = macro_;
      return data;
    }

  private:
    std::vector<std::vector<Symbol>> field_;
    std::vector<position_t> graph_positions_;
    std::map<position_t, std::pair<std::string, int>> potentials_;
    std::map<std::string, std::pair<std::string, int>> players_;
    std::map<int, Resource> resources_;
    std::map<int, Technology> technologies_;
    std::map<position_t, int> new_dead_neurons_;
    std::vector<bool> build_options_;
    std::vector<bool> synapse_options_;
    float audio_played_;
    int macro_;
    bool initialized_;
};

#endif
