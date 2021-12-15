#ifndef SRC_CLIENT_CONTEXT_H_
#define SRC_CLIENT_CONTEXT_H_

#include "share/eventmanager.h"
class ClientGame;

class Context {
  public: 
    /**
     * Empty constructor for use in f.e. maps.
     */
    Context() {}

    /**
     * Default constructor with one set of initial handlers.
     * @param[in] handlers to be added.
     */
    Context(std::map<char, void(ClientGame::*)(int)>& handlers) {
      for (const auto& it : handlers)
        eventmanager_.AddHandler(it.first, it.second);
    } 

    /**
     * Constructor with two sets of initial handlers. (Mainly to make
     * initializing easier, if multiple contexts use a set of identical
     * handlers.
     * @param[in] std_handlers to be added (standard handlers)
     * @param[in] handlers to be added (special handlers).
     */
    Context(std::map<char, void(ClientGame::*)(int)> std_handlers, std::map<char, void(ClientGame::*)(int)> handlers) {
      for (const auto& it : std_handlers)
        eventmanager_.AddHandler(it.first, it.second);
      for (const auto& it : handlers)
        eventmanager_.AddHandler(it.first, it.second);
    } 

    // getter

    EventManager<char, ClientGame, int>& eventmanager() { return eventmanager_; }

  private:
    EventManager<char, ClientGame, int> eventmanager_;
};

#endif
