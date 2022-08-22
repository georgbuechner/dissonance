#ifndef SRC_SHARE_DATA_H_
#define SRC_SHARE_DATA_H_

#include "nlohmann/json.hpp"
#include "share/defines.h"
#include <cstddef>
#include <iostream>
#include <memory>
#include <string>
#include <sstream>
#include <vector>

class FieldPosition;
class Update;
class StaticticsEntry;
class Statictics;

/**
 * Used for cmds: `preparing`, `select_mode`, `select_audio`, `send_audio`
 * `resign`, `set_pause_on`, `set_pause_off`, `initialize_user`, `ready`
 */
class Data {
  public: 
    Data();

    // objects
    struct Symbol {
      // attributes
      std::string symbol_;
      short color_;
      // methods
      std::string str() const;
      Symbol();
      Symbol(std::pair<std::string, short> symbol);
      std::pair<std::string, short> to_pair() const;
    };

    struct Resource {
      std::string value_;
      std::string bound_;
      std::string limit_;
      std::string iron_;
      bool active_;
      // methods
      Resource();
      Resource(double value, double bound, unsigned int limit, unsigned int iron, bool active);
      Resource(const char* payload, size_t len, size_t& offset);
      void binary(std::stringstream& buffer) const;
    };

    struct LobbyEntry {
      short _max_players;
      short _cur_players;
      std::string _game_id;
      std::string _audio_map_name;
      LobbyEntry(short max_players, short cur_players, std::string game_id, std::string audio_map_name);
      LobbyEntry(const char* payload, size_t len, size_t& offset);
      void binary(std::stringstream& buffer) const;
    };

    struct PositionInfo {
      PositionInfo() : _unit(-1), _pos({-1, -1}) {}
      PositionInfo(short unit) : _unit(unit), _pos({-1, -1}) {}
      PositionInfo(position_t pos) : _unit(-1), _pos(pos) {}
      PositionInfo(short unit, position_t pos) : _unit(unit), _pos(pos) {}

      short _unit;
      position_t _pos;
    };

    // getter 
    std::string u() { return u_; }

    virtual std::string msg() { return ""; } 

    virtual unsigned short mode() { return 0; }
    virtual unsigned short lines() { return 0; }
    virtual unsigned short cols() { return 0; }
    virtual unsigned short num_players() { return 0; }
    virtual std::string game_id() { return ""; }

    virtual position_t pos() const { return {-1, -1}; }
    virtual short unit() const { return -1; }
    virtual short color() const { return -1; }
    virtual std::vector<FieldPosition> units() const { return {}; }

    virtual std::map<std::string, std::pair<std::string, int>> players() { return {}; }
    virtual std::map<position_t, std::pair<std::string, short>> potentials() { return {}; }
    virtual std::map<position_t, int> new_dead_neurons() { return {}; }
    virtual float audio_played() { return 0; }
    virtual std::map<int, Resource> resources() { return {}; }
    virtual std::map<int, tech_of_t> technologies() { return {}; }
    virtual std::vector<bool> build_options() { return {}; }
    virtual std::vector<bool> synapse_options() { return {}; }

    virtual std::vector<std::vector<Data::Symbol>> field() { return {}; }
    virtual std::vector<position_t> graph_positions() { return {}; }
    virtual std::vector<position_t> centered_positions() { return {}; }
    virtual std::vector<position_t> current_way() { return {}; }
    virtual std::vector<position_t> current_waypoints() { return {}; }
    virtual std::vector<position_t> player_units() { return {}; }
    virtual std::vector<position_t> enemy_units() { return {}; }
    virtual std::vector<position_t> target_positions() { return {}; }
    virtual std::shared_ptr<Update> update() { return nullptr; }
    virtual short macro() { return 0; }
    virtual std::map<std::string, size_t> ai_strategies() { return {}; }

    virtual const std::vector<LobbyEntry> lobby() { return {}; }

    virtual std::string player_name() const { return ""; }
    virtual short player_color() const { return 0; }
    virtual std::map<unsigned short, unsigned short> neurons_build() const { return {}; }
    virtual std::map<unsigned short, unsigned short> potentials_build() const { return {}; }
    virtual std::map<unsigned short, unsigned short> potentials_killed() const { return {}; }
    virtual std::map<unsigned short, unsigned short> potentials_lost() const { return {}; }
    virtual unsigned short epsp_swallowed() const { return 0; }
    virtual std::map<int, std::map<std::string, double>> stats_resources() const { return {}; }
    virtual std::map<int, tech_of_t> stats_technologies() const { return {}; }
    virtual std::vector<StaticticsEntry> graph() const { return {}; }

    virtual std::vector<std::shared_ptr<Statictics>> statistics() { return {}; }

    virtual short num() { return -1; }
    virtual position_t start_pos() { return {-1, -1}; }
    virtual std::vector<position_t> positions() { return {}; }
    virtual short range() { return 0; }
    virtual position_t synapse_pos() { return {-1, -1}; }
    virtual position_t way_point() { return {-1, -1}; }
    virtual position_t target() { return {-1, -1}; }
    
    virtual std::string return_cmd() { return ""; }
    virtual std::map<short, PositionInfo> position_requests() { return {}; }
    virtual std::shared_ptr<Data> data() { return nullptr; }

    virtual bool same_device() { return false; }
    virtual std::string map_path() { return ""; }
    virtual std::string audio_file_name() { return ""; }

    virtual std::string username() { return ""; }
    virtual std::string songname() { return ""; }
    virtual std::string content() { return ""; }
    virtual int part() { return 0; } 
    virtual int parts() { return 0; }

    virtual::std::string map_name() { return ""; }
    virtual::std::map<std::string, nlohmann::json> ai_audio_data() { return {}; }

    virtual unsigned short resource() const { return 0; }
    virtual unsigned short technology() const { return 0; }

    virtual bool send_audio() { return false; }
    virtual bool send_ai_audios() { return false; }

    // setter
    virtual void set_num_players(unsigned short num_players) {}
    virtual void set_game_id(std::string game_id) {}
    virtual void set_resources(std::map<int, Resource> resources) {}
    virtual void set_build_options(std::vector<bool> build_options) {}
    virtual void set_synapse_options(std::vector<bool> synapse_options) {}
    virtual void set_macro(short macro) {}
    virtual void set_ai_strategies(std::map<std::string, size_t> def_strategies) {}
    virtual void set_pos(position_t pos) {}
    virtual void set_start_pos(position_t pos) {}
    virtual void set_positions(std::vector<position_t> positions) {}
    virtual void set_centered_positions(std::vector<position_t> positions) {} 
    virtual void set_current_way(std::vector<position_t> positions) {}
    virtual void set_current_waypoints(std::vector<position_t> positions) {}
    virtual void set_player_units(std::vector<position_t> positions) {}
    virtual void set_enemy_units(std::vector<position_t> positions) {}
    virtual void set_target_positions(std::vector<position_t> positions) {}

    virtual void set_range(short range) {}
    virtual void set_synapse_pos(position_t pos) {}
    virtual void set_way_point(position_t pos) {}
    virtual void set_target(position_t pos) {}
    virtual void set_num(short num) {}
    virtual void set_msg(std::string msg) {}

    virtual void set_content(std::string content) { }
    virtual void set_parts(int parts) { }
    virtual void set_part(int part) {}

    // methods 
    virtual void binary(std::stringstream&) {} 
    virtual void AddEntry(std::string game_id, short max_players, short cur_players, std::string audio_map_name) {}
    virtual void AddStatistics(std::shared_ptr<Statictics> statistics) {}
    /**
     * Used by Pick-Context to set selected ('picked') position. 
     * Function needs to exists for all data-structs dependant on Pick-Context.
     */
    virtual void SetPickedPosition(position_t pos) {}
    virtual void AddAiAudioData(std::string source_path, nlohmann::json analysed_data) {}

    void AddUsername(std::string username) { u_ = username; }

  private:
    std::string u_;
};

/**
 * Used for cmds: `kill`, `print_msg`, `set_msg`
 */
class Msg : public Data {
  public: 
    Msg(std::string msg);
    Msg(const char* payload, size_t len, size_t& offset);

    // getter 
    std::string msg();

    // methods 
    void binary(std::stringstream& buffer);

  private:
    std::string msg_;
};

/**
 * Used for commands: `init_game` (from client-side)
 */
class InitNewGame : public Data {
  public:
    InitNewGame(unsigned short mode, unsigned short lines, unsigned short cols);
    InitNewGame(const char* payload, size_t len, size_t& offset);

    // getter
    unsigned short mode();
    unsigned short lines();
    unsigned short cols();
    unsigned short num_players();
    std::string game_id();
    bool mc_ai();

    // setter 
    void set_num_players(unsigned short num_players);
    void set_game_id(std::string game_id);
    
    // methods 
    void binary(std::stringstream& buffer);

  private:
    unsigned short mode_;
    unsigned short lines_;
    unsigned short cols_;
    unsigned short num_players_; ///< only for mode=MULTI_PLAYER
    std::string game_id_; ///< only for mode=MULTI_PLAYER_CLIENT
};

/**
 * Used for cmds: `update_game`
 */
class Update : public Data {
  public: 
    Update(std::map<std::string, std::pair<std::string, int>> players, 
        std::map<position_t, std::pair<std::string, short>> potentials,
        std::map<position_t, int> new_dead_neurons, 
        float audio_played);
    Update(const char* payload, size_t len, size_t& offset);

    // getter 
    std::map<std::string, std::pair<std::string, int>> players();
    std::map<position_t, std::pair<std::string, short>> potentials();
    std::map<position_t, int> new_dead_neurons();
    float audio_played();
    std::map<int, Resource> resources();
    std::vector<bool> build_options();
    std::vector<bool> synapse_options();

    // setter 
    void set_resources(std::map<int, Resource> resources);
    void set_build_options(std::vector<bool> build_options);
    void set_synapse_options(std::vector<bool> synapse_options);
    
    // methods 
    void binary(std::stringstream& buffer);
    t_topline PlayersToPrint() {
      t_topline print;
      for (const auto& it : players_) {
        print.push_back({it.first + ": " + it.second.first, it.second.second});
        print.push_back({" | ", COLOR_DEFAULT});
      }
      if (print.size() > 0)
        print.pop_back();
      return print;
    }

  private: 
    // Identical for all players
    std::map<std::string, std::pair<std::string, int>> players_;
    std::map<position_t, std::pair<std::string, short>> potentials_;
    std::map<position_t, int> new_dead_neurons_;
    float audio_played_;

    // player-specific-data
    std::map<int, Resource> resources_;
    std::vector<bool> build_options_;
    std::vector<bool> synapse_options_;
};

/** 
 * Used for cmds: `init_game` (TODO (fux): change to `init_field`?)
 */
class Init : public Data {
  public: 
    Init(std::shared_ptr<Update> update, std::vector<std::vector<Data::Symbol>> field, 
       std::vector<position_t> graph_positions, std::map<int, tech_of_t> technologies);
    Init(const char* payload, size_t len, size_t& offset);

    // getter 
    std::vector<std::vector<Symbol>> field();
    std::vector<position_t> graph_positions();
    std::map<int, tech_of_t> technologies();
    std::shared_ptr<Update> update();
    short macro();
    std::map<std::string, size_t> ai_strategies();

    // setter 
    void set_macro(short macro);
    void set_ai_strategies(std::map<std::string, size_t> def_strategies);
    
    // methods 
    void binary(std::stringstream& buffer);

  private:
    // Identical for all players
    std::vector<std::vector<Data::Symbol>> field_;
    std::vector<position_t> graph_positions_;
    std::map<int, tech_of_t> technologies_;
    std::shared_ptr<Update> update_;
    std::map<std::string, size_t> ai_strategies_;
    
    // player-specific-data
    short macro_;
};

/**
 * Used for command `set_unit`
 */
class FieldPosition: public Data {
  public: 
    FieldPosition(position_t pos, short unit, short color);
    FieldPosition(position_t pos, short unit, short color, unsigned short resource);
    FieldPosition(const char* payload, size_t len, size_t& offset);

    // getter 
    position_t pos() const;
    short unit() const;
    short color() const;
    unsigned short resource() const;
    
    // methods 
    void binary(std::stringstream& buffer);

  private: 
    position_t pos_;
    short unit_;
    short color_;
    short resource_;
};

/**
 * Used for command `set_units`
 */
class Units : public Data {
  public: 
    Units(std::vector<FieldPosition> units);
    Units(const char* payload, size_t len, size_t& offset);

    // getter 
    std::vector<FieldPosition> units() const;

    // methods 
    void binary(std::stringstream& buffer);

  private: 
    std::vector<FieldPosition> units_; // TODO (fux): consider using shared_ptrs
};

/**
 * Used for command `update_lobby`
 */
class Lobby  : public Data {
  public:
    Lobby();
    Lobby(const char* payload, size_t len, size_t& offset);

    // getter 
    const std::vector<LobbyEntry> lobby();

    // methods    
    void AddEntry(std::string game_id, short max_players, short cur_players, std::string audio_map_name);
    void clear();
    void binary(std::stringstream& buffer);

  private:
    std::vector<LobbyEntry> lobby_;
};

class Statictics : public Data {
  public:
    Statictics();
    Statictics(const char* payload, size_t len, size_t& offset);
    Statictics(const Statictics& statistics);

    // getter
    std::string player_name() const;
    short player_color() const;
    std::map<unsigned short, unsigned short> neurons_build() const;
    std::map<unsigned short, unsigned short> potentials_build() const;
    std::map<unsigned short, unsigned short> potentials_killed() const;
    std::map<unsigned short, unsigned short> potentials_lost() const;
    unsigned short epsp_swallowed() const;
    std::map<int, std::map<std::string, double>> stats_resources() const;
    std::map<int, tech_of_t> stats_technologies() const;
    std::map<int, std::map<std::string, double>>& stats_resources_ref();
    std::vector<StaticticsEntry> graph() const;

    // setter (no virtual, as called directly from player
    void set_player_name(std::string player_name);
    void set_color(short color);
    void set_technologies(std::map<int, tech_of_t> technologies);

    // methods
    void AddNewNeuron(int unit);
    void AddNewPotential(int unit);
    void AddKillderPotential(std::string id);
    void AddLostPotential(std::string id);
    void AddEpspSwallowed();
    void AddStatisticsEntry(double oxygen, double potassium, double chloride, double glutamate, 
        double dopamine, double serotonin);

    void print();
    void binary(std::stringstream& buffer);

  private:
    std::string player_name_;
    short player_color_;
    std::map<unsigned short, unsigned short> neurons_build_;
    std::map<unsigned short, unsigned short> potentials_build_;
    std::map<unsigned short, unsigned short> potentials_killed_;
    std::map<unsigned short, unsigned short> potentials_lost_;
    unsigned short epsp_swallowed_;
    std::map<int, std::map<std::string, double>> resources_;
    std::map<int, tech_of_t> technologies_;
    std::vector<StaticticsEntry> graph_;

    std::vector<int> tmp_neurons_built_; ///< not included when packing
};

class StaticticsEntry : public Data {
  public: 
    StaticticsEntry(const char* payload, size_t len, size_t& offset);
    StaticticsEntry(double oxygen, double potassium, double chloride, double glutamate, double dopamine, double serotonin,
        std::vector<int> neurons_built);

    // getter 
    double oxygen();
    double potassium();
    double chloride();
    double glutamate();
    double dopamine();
    double serotonin();
    std::vector<int> neurons_built();

    // methods 
    void binary(std::stringstream& buffer);

  private:
    double oxygen_;
    double potassium_;
    double chloride_;
    double glutamate_;
    double dopamine_;
    double serotonin_;
    std::vector<int> neurons_built_;
};

/**
 * Used for command `game_end`
 */
class GameEnd : public Data {
  public:
    GameEnd(std::string msg);
    GameEnd(const char* payload, size_t len, size_t& offset);

    // getter 
    std::string msg();
    std::vector<std::shared_ptr<Statictics>> statistics();

    // setter
    void set_msg(std::string msg);
    
    // methods 
    void AddStatistics(std::shared_ptr<Statictics> statistics);
    void binary(std::stringstream& buffer);

  private: 
    std::string msg_;
    std::vector<std::shared_ptr<Statictics>> statistics_;
};

/**
 * Used for command `build_potential` and `check_build_potential`
 */
class BuildPotential : public Data {
  public: 
    BuildPotential(short unit, short num);
    BuildPotential(const char* payload, size_t len, size_t& offset);
    
    // getter 
    short unit() const;
    short num();
    position_t synapse_pos();
    std::vector<position_t> positions();

    // setter 
    void set_start_pos(position_t pos);
    void set_positions(std::vector<position_t> positions);

    // methods
    void binary(std::stringstream& buffer);

    /**
     * Used by Pick-Context to set selected ('picked') position. 
     * Function needs to exists for all data-structs dependant on Pick-Context.
     * Here: synapse_pos
     */
    void SetPickedPosition(position_t pos);

  private: 
    short unit_;
    short num_;
    position_t synapse_pos_; ///< position from which to start field-context (nucleus-position)
    std::vector<position_t> positions_;
};

/**
 * Used for command `build_neuron`
 */
class BuildNeuron : public Data {
  public: 
    BuildNeuron(short unit);
    BuildNeuron(const char* payload, size_t len, size_t& offset);
    
    // getter 
    short unit() const;
    position_t pos() const ;
    position_t start_pos();
    std::vector<position_t> positions();
    short range();

    // setter 
    void set_pos(position_t pos);
    void set_start_pos(position_t pos);
    void set_positions(std::vector<position_t> positions);
    void set_range(short range);

    // methods
    void binary(std::stringstream& buffer);

    /**
     * Used by Pick-Context to set selected ('picked') position. 
     * Function needs to exists for all data-structs dependant on Pick-Context.
     * Here: start_pos_
     */
    void SetPickedPosition(position_t pos);

  private: 
    short unit_;
    short range_;
    position_t pos_; ///< pos where to build
    position_t start_pos_;  ///< position from which to start field-context (nucleus-position)
    std::vector<position_t> positions_;
};

class SelectSynapse : public Data {
  public: 
    SelectSynapse();
    SelectSynapse(position_t synapse_pos);
    SelectSynapse(std::vector<position_t> positions);
    SelectSynapse(const char* payload, size_t len, size_t& offset);

    // getter 
    position_t synapse_pos();
    std::vector<position_t> player_units();

    // setter 
    void set_synapse_pos(position_t pos);
    void set_player_units(std::vector<position_t> positions);

    // methods
    void binary(std::stringstream& buffer);

    /**
     * Used by Pick-Context to set selected ('picked') position. 
     * Function needs to exists for all data-structs dependant on Pick-Context.
     * Here: synapse_pos_
     */
    void SetPickedPosition(position_t pos);

  private:
    position_t synapse_pos_;
    std::vector<position_t> player_units_;
};

class SetWayPoints : public Data {
  public: 
    SetWayPoints(position_t synapse_pos);
    SetWayPoints(position_t synapse_pos, short num);
    SetWayPoints(const char* payload, size_t len, size_t& offset);

    // getter 
    position_t synapse_pos();
    position_t start_pos();
    position_t way_point();
    std::vector<position_t> centered_positions();
    std::vector<position_t> current_way();
    std::vector<position_t> current_waypoints();
    std::string msg();
    short num();

    // setter 
    void set_way_point(position_t pos);
    void set_centered_positions(std::vector<position_t> positions);
    void set_current_way(std::vector<position_t> positions);
    void set_current_waypoints(std::vector<position_t> positions);
    void set_msg(std::string msg);
    void set_num(short num);

    // methods
    void binary(std::stringstream& buffer);

    /**
     * Used by Pick-Context to set selected ('picked') position. 
     * Function needs to exists for all data-structs dependant on Pick-Context.
     * Here: way_point_
     */
    void SetPickedPosition(position_t pos);

  private:
    position_t synapse_pos_;
    position_t start_pos_;
    position_t way_point_;
    std::vector<position_t> centered_positions_;
    std::vector<position_t> current_way_;
    std::vector<position_t> current_waypoints_;
    std::string msg_;
    short num_;
};

class SetTarget : public Data {
  public: 
    SetTarget(position_t synapse_pos, short unit);
    SetTarget(const char* payload, size_t len, size_t& offset);

    // getter 
    short unit() const;
    position_t synapse_pos();
    position_t start_pos(); 
    position_t target();
    std::vector<position_t> enemy_units();
    std::vector<position_t> target_positions();

    // setter 
    void set_start_pos(position_t pos);
    void set_target(position_t pos);
    void set_enemy_units(std::vector<position_t> positions);
    void set_target_positions(std::vector<position_t> positions);

    // methods
    void binary(std::stringstream& buffer);

    /**
     * Used by Pick-Context to set selected ('picked') position. 
     * Function needs to exists for all data-structs dependant on Pick-Context.
     * Here: start_pos_
     */
    void SetPickedPosition(position_t pos);

  private:
    short unit_;
    position_t synapse_pos_;
    position_t start_pos_; ///< position from which to start field-context (random field position)
    position_t target_;
    std::vector<position_t> enemy_units_;
    std::vector<position_t> target_positions_;
};

class ToggleSwarmAttack : public Data {
  public: 
    ToggleSwarmAttack(position_t synapse_pos);
    ToggleSwarmAttack(const char* payload, size_t len, size_t& offset);

    // getter 
    position_t synapse_pos();

    // methods
    void binary(std::stringstream& buffer);

  private: 
    position_t synapse_pos_;
};

class GetPositions : public Data {
  public:
    GetPositions(std::string return_cmd, std::map<short, PositionInfo> position_requests, std::shared_ptr<Data> data);
    GetPositions(const char* payload, size_t len, size_t& offset);

    // getter
    std::string return_cmd();
    std::map<short, PositionInfo> position_requests();
    std::shared_ptr<Data> data();

    // methods
    void binary(std::stringstream& buffer);

  private:
    std::string return_cmd_; ///< might be one of: `select_synapse`, `set_wps`, `target`
    std::map<short, PositionInfo> position_requests_;
    std::shared_ptr<Data> data_;
};

/**
 * Used for commands: `audio_map`, `audio_exists` (ignores `same_device_`)
 */
class CheckSendAudio : public Data {
  public:
    CheckSendAudio(std::string map_path);
    CheckSendAudio(std::string map_path, std::string audio_file_name);
    CheckSendAudio(const char* payload, size_t len, size_t& offset);

    // getter 
    bool same_device();
    std::string map_path();
    std::string audio_file_name();
    
    // methods
    void binary(std::stringstream& buffer);

  private:
    bool same_device_;
    std::string map_path_;
    std::string audio_file_name_;
};

/** 
 * Used for commands: `send_audio_info`
 */
class SendAudioInfo : public Data {
  public:
    SendAudioInfo(bool send_audio, bool send_ai_audios);
    SendAudioInfo(const char* payload, size_t len, size_t& offset);

    // getter
    bool send_audio();
    bool send_ai_audios();
    
    // methods
    void binary(std::stringstream& buffer);

  private:
    bool send_audio_;
    bool send_ai_audios_;
};

class AudioTransferData : public Data {
  public:
    AudioTransferData(std::string username, std::string audioname);
    AudioTransferData(const char* payload, size_t len, size_t& offset);

    // getter
    std::string username();
    std::string songname();
    std::string content();
    int part();
    int parts();

    // setter 
    void set_content(std::string content);
    void set_parts(int parts);
    void set_part(int part);

    // Methods 
    void binary(std::stringstream& buffer);

  private: 
    std::string username_;
    std::string audioname_;
    std::string content_;
    int part_;
    int parts_;
};

/**
 * Used for command: `initialize_game`
 */
class InitializeGame : public Data {
  public: 
    InitializeGame(std::string map_name);
    InitializeGame(const char* payload, size_t len, size_t& offset);

    // getter 
    std::string map_name();
    std::map<std::string, nlohmann::json> ai_audio_data();

    // methods
    void binary(std::stringstream& buffer);
    void AddAiAudioData(std::string source_path, nlohmann::json analysed_data);

  private:
    std::string map_name_;
    std::map<std::string, nlohmann::json> ai_audio_data_;
};

/**
 * Used for commands: `add_iron` and `remove_iron`
 */
class DistributeIron : public Data {
  public:
    DistributeIron(unsigned short resource);
    DistributeIron(const char* payload, size_t len, size_t& offset);

    // getter
    unsigned short resource() const;

    // methods
    void binary(std::stringstream& buffer);

  private:
    unsigned short resource_;
};

class AddTechnology : public Data {
  public:
    AddTechnology(unsigned short technology);
    AddTechnology(const char* payload, size_t len, size_t& offset);

    // getter
    unsigned short technology() const;

    // methods
    void binary(std::stringstream& buffer);

  private:
    unsigned short technology_;
};

#endif
