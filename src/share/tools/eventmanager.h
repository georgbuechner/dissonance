#ifndef SRC_CLIENT_EVENTMANAGER_H_
#define SRC_CLIENT_EVENTMANAGER_H_

#include <map>
#include <string>
class ClientGame;

template<class T_event, class T_function, class T_param>
class EventManager {
  public:
    EventManager() {};

    // getter 
    std::map<T_event, void(T_function::*)(T_param)>& handlers() {
      return handlers_;
    }

    std::string options() {
      std::string options = "";
      for (const auto& it : handlers_) {
        if (it.first == '\n')
          options += "[enter]";
        else 
          options += it.first;
        options += ", ";
      }
      return options;
    }

    void AddHandler(T_event event, void(T_function::*handler)(T_param)) {
      handlers_[event] = handler;
    }
    void RemoveHandler(T_event event) {
      if (handlers_.count(event) > 0)
        handlers_.erase(event);
    }

  private:
    std::map<T_event, void(T_function::*)(T_param)> handlers_;
};

#endif
