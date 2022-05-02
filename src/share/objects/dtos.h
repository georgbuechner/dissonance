#ifndef SRC_SHARE_OBJECTS_DTOS_
#define SRC_SHARE_OBJECTS_DTOS_

#include "nlohmann/json.hpp"
#include "share/defines.h"
#include "share/tools/utils/utils.h"
#include <iostream>
#include <msgpack/sbuffer.h>
#include <msgpack/unpack.h>
#include <string>
#include <sstream>
#include <utility>
#include <vector>
#include <msgpack/pack.h>

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
      msgpack_zone mempool;
      msgpack_zone_init(&mempool, 2048);

      username_ = load_string(payload, len, offset, &mempool);
      audioname_ = load_string(payload, len, offset, &mempool);
      part_ = load_int(payload, len, offset, &mempool);
      parts_ = load_int(payload, len, offset, &mempool);
      content_ = load_string(payload, len, offset, &mempool);
      msgpack_zone_destroy(&mempool);
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
      msgpack_sbuffer sbuf;
      msgpack_sbuffer_init(&sbuf);
      
      std::stringstream buffer;
      /* serialize values into the buffer using msgpack_sbuffer_write callback function. */
      msgpack_packer pk;
      msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);

      // username
      msgpack_pack_str(&pk, username_.length());
      msgpack_pack_str_body(&pk, username_.c_str(), username_.length());
      // audioname
      msgpack_pack_str(&pk, audioname_.length());
      msgpack_pack_str_body(&pk, audioname_.c_str(), audioname_.length());
      // part and parts
      msgpack_pack_int(&pk, part_);
      msgpack_pack_int(&pk, parts_);
      // content 
      msgpack_pack_str(&pk, content_.length());
      msgpack_pack_str_body(&pk, content_.c_str(), content_.length());

      buffer.write(sbuf.data,sbuf.size);
      msgpack_sbuffer_destroy(&sbuf);
      return buffer.str();
    }

  private: 
    std::string username_;
    std::string audioname_;
    std::string content_;
    int part_;
    int parts_;

    std::string load_string(const char* payload, size_t len, size_t& offset, msgpack_zone *mempool) {
      msgpack_object deserialized;
      if(msgpack_unpack(payload, len, &offset, mempool, &deserialized)!=0) {
        if(deserialized.type == MSGPACK_OBJECT_STR) {
          return std::string(deserialized.via.str.ptr,deserialized.via.str.size);
        }
        else {
          throw std::invalid_argument("The next expected type was string");
        }
      }
      else {
        throw std::invalid_argument("Unpack encountered an error");
      }
    }

    uint64_t load_int(const char* payload, size_t len, size_t& offset, msgpack_zone *mempool) {
      msgpack_object deserialized;
      if(msgpack_unpack(payload, len, &offset, mempool, &deserialized)!=0) {
        if(deserialized.type == MSGPACK_OBJECT_POSITIVE_INTEGER) {
          return deserialized.via.u64;
        }
        else {
          throw std::invalid_argument("The next expected type was positive integer");
        }
      }
      else {
        throw std::invalid_argument("Unpack encountered an error");
      }
    }

    void write_int(int x, std::stringstream &buffer) {
      msgpack_sbuffer sbuf;
      msgpack_sbuffer_init(&sbuf);

      /* serialize values into the buffer using msgpack_sbuffer_write callback function. */
      msgpack_packer pk;
      msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);
      msgpack_pack_int(&pk,x);

      buffer.write(sbuf.data,sbuf.size);
      msgpack_sbuffer_destroy(&sbuf);
    }
};

#endif
