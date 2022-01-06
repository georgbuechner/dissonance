#ifndef SRC_CLIENT_CONTEXT_H_
#define SRC_CLIENT_CONTEXT_H_

#include "nlohmann/json.hpp"
#include "share/defines.h"
#include "share/tools/eventmanager.h"
#include <vector>

class ClientGame;

class Context {
  public: 
    /**
     * Empty constructor for use in f.e. maps.
     */
    Context() : msg_("") {}

    /**
     * Default constructor with one set of initial handlers.
     * @param[in] handlers to be added.
     */
    Context(std::string msg, std::map<char, void(ClientGame::*)(nlohmann::json&)>& handlers,
        std::vector<std::pair<std::string, int>> topline) : msg_(msg) {
      for (const auto& it : handlers)
        eventmanager_.AddHandler(it.first, it.second);
      topline_ = topline;
    } 

    /**
     * Constructor with two sets of initial handlers. (Mainly to make
     * initializing easier, if multiple contexts use a set of identical
     * handlers.
     * @param[in] std_handlers to be added (standard handlers)
     * @param[in] handlers to be added (special handlers).
     */
    Context(std::string msg, std::map<char, void(ClientGame::*)(nlohmann::json&)> std_handlers, 
        std::map<char, void(ClientGame::*)(nlohmann::json&)> handlers,
        std::vector<std::pair<std::string, int>> topline) : msg_(msg) {
      for (const auto& it : std_handlers)
        eventmanager_.AddHandler(it.first, it.second);
      for (const auto& it : handlers)
        eventmanager_.AddHandler(it.first, it.second);
      topline_ = topline;
    } 

    // getter

    std::string msg() const { return msg_; }
    int current_unit() const { return current_unit_; }
    int current_range() const { return current_range_; }
    position_t current_pos() const { return current_pos_; }
    char cmd() const { return cmd_; }
    nlohmann::json data() const { return data_; }
    std::string action() const { return action_; }

    std::vector<std::pair<std::string, int>> topline() const { return topline_; }
    EventManager<char, ClientGame, nlohmann::json&>& eventmanager() { return eventmanager_; }

    // setter 
    void set_data(nlohmann::json data) { data_ = data; }
    void set_cmd(char cmd) { cmd_ = cmd; }
    void set_action(std::string action) { action_ = action; }
    
  private:
    EventManager<char, ClientGame, nlohmann::json&> eventmanager_;
    std::string action_;
    std::string msg_;
    int current_unit_;
    int current_range_;
    position_t current_pos_;

    char cmd_;
    nlohmann::json data_;

    std::vector<std::pair<std::string, int>> topline_;
};

#endif
