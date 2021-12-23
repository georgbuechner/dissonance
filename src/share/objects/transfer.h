#ifndef SRC_OBJECT_TRANSFER_H_
#define SRC_OBJECT_TRANSFER_H_
#include <string>
#include <vector>

#include "nlohmann/json.hpp"
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
      spdlog::get(LOGGER)->info("from json: field {}", json["field"].dump());
      for (const auto& it : json["field"].get<std::vector<nlohmann::json>>()) {
        std::vector<Symbol> vec;
        for (const auto& jt : it.get<std::vector<nlohmann::json>>())
          vec.push_back({jt["symbol"].get<std::string>(), jt["color"].get<int>()});
        field_.push_back(vec);
      }

      // build resources from json.
      spdlog::get(LOGGER)->info("from json: resources {}", json["resources"].dump());
      if (json.contains("resources")) {
        for (const auto& it : json["resources"].get<std::map<std::string, nlohmann::json>>()) {
          resources_[stoi(it.first)] = {it.second["value"], it.second["bound"], it.second["limit"], 
            it.second["iron"], it.second["active"]};
        }
      }

      // build technologies from json.
      spdlog::get(LOGGER)->info("from json: technologies {}", json["technologies"].dump());
      if (json.contains("technologies")) {
        for (const auto& it : json["technologies"].get<std::map<std::string, nlohmann::json>>())
          technologies_[stoi(it.first)] = {it.second["cur"], it.second["max"], it.second["active"]};
      }

      // build potentials from string 
      spdlog::get(LOGGER)->info("from json: technologies {}", json["technologies"].dump());
      if (json.contains("potentials")) {
        for (const auto& it : json["potentials"].get<std::map<std::string, nlohmann::json>>())
          potentials_[utils::PositionFromString(it.first)] = {it.second["symbol"].get<std::string>(), 
            it.second["color"].get<int>()};
      }

      // Audio Played
      if (json.contains("audio_played"))
        audio_played_ = json["audio_played"];

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
    void set_potentials(std::map<position_t, std::pair<std::string, int>> potentials) {
      potentials_ = potentials;
    }
    void set_audio_played(float audio_played) {
      audio_played_ = (audio_played < 0) ? 0 : audio_played;
    }

    nlohmann::json json() {
      nlohmann::json data; 
      data["players"] = players_;
      data["field"] = nlohmann::json::array();
      for (const auto& it : field_) {
        nlohmann::json vec = nlohmann::json::array();
        for (const auto& jt : it)
          vec.push_back({{"symbol", jt.symbol_}, {"color", jt.color_}});
        data["field"].push_back(vec);
      }
      data["resources"] = nlohmann::json::object();
      for (const auto& it : resources_) {
        data["resources"][std::to_string(it.first)] = {{"value", it.second.value_}, {"bound", it.second.bound_}, 
          {"limit", it.second.limit_}, {"iron", it.second.iron_}, {"active", it.second.active_}};
      }
      data["technologies"] = nlohmann::json::object();
      for (const auto& it : technologies_) {
        data["technologies"][std::to_string(it.first)] = {{"cur", it.second.cur_}, {"max", it.second.max_}, 
          {"active", it.second.active_}};
      }
      data["audio_played"] = audio_played_;
      data["potentials"] = nlohmann::json::object();
      for (const auto& it : potentials_) {
        data["potentials"][utils::PositionToString(it.first)] = {
          {"symbol", it.second.first}, {"color", it.second.second}};
      }
      return data;
    }

  private:
    std::vector<std::vector<Symbol>> field_;
    std::map<position_t, std::pair<std::string, int>> potentials_;
    std::map<std::string, std::string> players_;
    std::map<int, Resource> resources_;
    std::map<int, Technology> technologies_;
    float audio_played_;
    bool initialized_;
};

#endif
