#ifndef SRC_CLIENT_CONTEXT_H_
#define SRC_CLIENT_CONTEXT_H_

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
    Context(std::string msg, std::map<char, void(ClientGame::*)(int)>& handlers,
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
    Context(std::string msg, std::map<char, void(ClientGame::*)(int)> std_handlers, 
        std::map<char, void(ClientGame::*)(int)> handlers,
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
    std::vector<std::pair<std::string, int>> topline() const { return topline_; }
    EventManager<char, ClientGame, int>& eventmanager() { return eventmanager_; }

    // setter 
    void set_current_unit(int unit) { current_unit_ = unit; }
    void set_current_range(int range) { current_range_ = range; }
    void set_current_pos(position_t pos) { current_pos_ = pos; }

  private:
    EventManager<char, ClientGame, int> eventmanager_;
    std::string msg_;
    int current_unit_;
    int current_range_;
    position_t current_pos_;
    std::vector<std::pair<std::string, int>> topline_;
};

#endif
