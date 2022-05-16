#ifndef SRC_SHARE_COMMANDS_H_
#define SRC_SHARE_COMMANDS_H_

#include <cstddef>
#include <iostream>
#include <memory>
#include <sstream>
#include "share/shemes/data.h"

class Command {
  public: 
    Command(std::string command);
    Command(std::string command, std::shared_ptr<Data> data);

    Command(const char* payload, size_t len);

    // getter 
    std::string command();
    std::string username();
    std::shared_ptr<Data> data();

    // setter 
    void set_username(std::string username);

    // methods 
    std::string bytes();

  private:
    std::string command_;
    std::string username_;
    std::shared_ptr<Data> data_;
};

#endif
