#ifndef SRC_SERVER_GAME_PLAYER_MONTOCARLOTREE_H_
#define SRC_SERVER_GAME_PLAYER_MONTOCARLOTREE_H_

#include "share/defines.h"
#include "share/tools/utils/utils.h"
#include <iostream>
#include <list>
#include <vector>

struct Node {
  float cur_weight_;
  std::map<std::string, Node*> _nodes;
};

class Graph {
  public:
    Graph() {
      _start = new Node({0xFFF});
      _cur = _start;
    }
    Graph(std::string path) {
      auto j = utils::LoadJsonFromDisc(path);
      _start = new Node({j["w"].get<float>()});
      _cur = _start;
      while (j.contains("n")) {
        for (const auto& it : j["n"].get<std::vector<nlohmann::json>>()) {
          AddNode(it["t"], it["w"]);
        }
      }
    }

    // methods
    void AddNode(std::string action, float weight) {
      if (_cur->_nodes.count(action) != 0)
        _cur->_nodes[action] = new Node({weight});
    }

    Node* GetNext(std::string action) {
      Node* rtn = _cur;
      _cur = _cur->_nodes.at(action);
      _visited_nodes.push_back(action);
      return rtn;
    }

    void UpdateWeights(float weight) {
      Node* next = _start;
      for (const auto& node : _visited_nodes) {
        next = _cur->_nodes.at(node);
        next->cur_weight_ = (weight + next->cur_weight_) / 2;
      }
    }

  private:
    Node* _start;
    Node* _cur;
    std::vector<std::string> _visited_nodes;
};

#endif 
