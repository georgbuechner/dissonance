#ifndef SRC_SHARE_OBJECTS_DTOS_
#define SRC_SHARE_OBJECTS_DTOS_

#include "nlohmann/json.hpp"
#include "share/defines.h"
#include "share/tools/utils/utils.h"
#include <iostream>
#include <string>
#include <sstream>
#include <utility>
#include <vector>
#include <msgpack.hpp>

struct GetPositionInfo {
  public:
    GetPositionInfo() : unit_(-1), pos_({-1, -1}) {}
    GetPositionInfo(int unit) : unit_(unit), pos_({-1, -1}) {}
    GetPositionInfo(position_t pos) : unit_(-1), pos_(pos) {}
    GetPositionInfo(int unit, position_t pos) : unit_(unit), pos_(pos) {}
    GetPositionInfo(nlohmann::json json) : unit_(json["unit"]), pos_(json["pos"]) {} 
   
    // getter
    int unit() const { return unit_; }
    position_t pos() const { return pos_; }

    // methods 
    nlohmann::json json() const {
      return nlohmann::json({{"unit", unit_}, {"pos", pos_}});
    }

  private:
    const int unit_;
    const position_t pos_;
};

class Dto {
  public: 
    Dto(std::string command, std::string username) : command_(command), username_(username) { }

    // getter 
    std::string username() { return username_; }
    std::string command() { return command_; }
    virtual std::string return_cmd() { return ""; };
    virtual std::map<int, GetPositionInfo> position_requests() { return {}; }
    
    // setter
    
    // methods
    virtual nlohmann::json json() const {
      return {{"command", command_}, { "username", username_}, {"data", nlohmann::json()}};
    }

  private:
    std::string command_;
    std::string username_;
};


class GetPosition : Dto {
  public: 
    GetPosition(std::string username, std::string return_cmd, 
        std::map<int, GetPositionInfo> position_requests) 
        : Dto("get_positions", username), return_cmd_(return_cmd), position_requests_(position_requests) {}
    GetPosition(nlohmann::json json) 
        : Dto(json["command"], json["username"]), return_cmd_(json["data"]["return_cmd"]) {
      for (const auto& it : json["data"]["position_requests"].get<std::map<std::string, nlohmann::json>>()) {
        position_requests_.insert(std::make_pair(std::stoi(it.first), GetPositionInfo(it.second)));
      }
    }
    
    // getter    
    std::string return_cmd() { return return_cmd_; };
    std::map<int, GetPositionInfo> position_requests() { return position_requests_; }

    // methods 
    nlohmann::json json() const {
      nlohmann::json json = Dto::json();
      json["data"]["return_cmd"] = return_cmd_;
      json["data"]["position_requests"] = nlohmann::json();
      for (const auto& it : position_requests_)
        json["data"]["position_requests"][std::to_string(it.first)] = it.second.json();
      return json;
    }

  private: 
    const std::string return_cmd_;
    std::map<int, GetPositionInfo> position_requests_;
};

class AudioTransferData {
  public:
    AudioTransferData(std::string username, std::string audioname) {
      // replace all ';' in user- and audioname with '_'
      username_ = username;
      audioname_ = audioname;
      // default init other members
      content_ = "";
      part_ = 0;
      parts_ = 0;
    }

    AudioTransferData(const char* payload, size_t len) {
      size_t offset = 0;
      msgpack::object_handle result;

      unpack(result, payload, len, offset);
      username_ = result->as<std::string>();

      unpack(result, payload, len, offset);
      audioname_ = result->as<std::string>();

      unpack(result, payload, len, offset);
      part_ = result->as<int>();

      unpack(result, payload, len, offset);
      parts_ = result->as<int>();

      unpack(result, payload, len, offset);
      content_ = result->as<std::string>();
    }

    // getter
    std::string username() { return username_; }
    std::string songname() { return audioname_; }
    std::string content() { return content_; }
    int part() { return part_; } 
    int parts() { return parts_; }

    // setter 
    void set_content(std::string content) { content_ = content; }
    void set_parts(int parts) { parts_ = parts; }
    void set_part(int part) { part_ = part; }

    // Methods 
    std::string string() {
      std::stringstream buffer;

      /* serialize values into the buffer using function. */
      msgpack::pack(buffer, username_);
      msgpack::pack(buffer, audioname_);
      msgpack::pack(buffer, part_);
      msgpack::pack(buffer, parts_);
      msgpack::pack(buffer, content_);

      return buffer.str();
    }

  private: 
    std::string username_;
    std::string audioname_;
    std::string content_;
    int part_;
    int parts_;
};

#endif
