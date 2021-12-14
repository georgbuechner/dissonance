#ifndef SRC_CLIENT_EVENTMANAGER_H_
#define SRC_CLIENT_EVENTMANAGER_H_

#include <map>
class ClientGame;

class EventManager {
  public:
    EventManager();

    // getter 
    std::map<char, bool(ClientGame::*)()>& handlers() {
      return handlers_;
    }

    void AddHandler(char event, bool(ClientGame::*handler)()) {
      handlers_[event] = handler;
    }

  private:
    std::map<char, bool(ClientGame::*)()> handlers_;
};

#endif
