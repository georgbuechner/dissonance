# ifndef SRC_SHARE_OBJECTS_LOBBY_H_
# define SRC_SHARE_OBJECTS_LOBBY_H_

#include "nlohmann/json.hpp"
#include "nlohmann/json_fwd.hpp"
#include <iostream>
#include <vector>

#define L_MAX "0"
#define L_CUR "1"
#define L_ID "2"
#define L_NAME "3"

class Lobby {
  private:
    struct LobbyEntry {
      int _max_players;
      int _cur_players;
      std::string _game_id;
      std::string _audio_map_name;
    };
    std::vector<LobbyEntry> lobby_;

  public:
    Lobby() { lobby_ = {}; }
    Lobby(nlohmann::json json) { 
      for (const auto& it : json.get<std::vector<nlohmann::json>>()) 
        lobby_.push_back(LobbyEntry({it[L_MAX], it[L_CUR], it[L_ID], it[L_NAME]}));
    }

    // getter 
    const std::vector<LobbyEntry>& lobby() {
      return lobby_;
    }

    void AddEntry(std::string game_id, int max_players, int cur_players, std::string audio_map_name) {
      lobby_.push_back(LobbyEntry({max_players, cur_players, game_id, audio_map_name}));
    }

    nlohmann::json ToJson() {
      nlohmann::json json = nlohmann::json::array();
      for (const auto& it : lobby_)  {
        nlohmann::json entry;
        entry[L_ID] = it._game_id;
        entry[L_MAX] = it._max_players;
        entry[L_CUR] = it._cur_players;
        entry[L_NAME] = it._audio_map_name;
        json.push_back(entry);
      }
      return json;
    }
};

#endif

