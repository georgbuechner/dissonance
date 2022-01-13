#ifndef SRC_GRAPH_H_
#define SRC_GRAPH_H_

#include <algorithm>
#include <curses.h>
#include <iostream>
#include <utility>
#include <list>
#include <map>
#include <vector>

struct Node {
  int line_;
  int col_;
  std::list<Node*> nodes_;
};

typedef std::pair<int, int> position_t;

class Graph {
  public:
    Graph() { }

    // getter:
    const std::map<position_t, Node*>& nodes() const {
      return nodes_; 
    }

    void AddNode(int line, int col) {
      position_t pos = {line, col};
      nodes_[pos] = new Node({line, col, {}});
    };

    void AddEdge(Node* a, Node* b) {
      a->nodes_.push_back(b);
    }

    bool InGraph(position_t pos) const {
      return nodes_.count(pos) > 0;
    }

    position_t GetPosNotInComponent(std::vector<position_t> component) {
      for (const auto& it : nodes_) {
        if (std::find(component.begin(), component.end(), it.first) == component.end())
          return it.first;
      }
      return {-1, -1};
    }

    /**
     * Removes all components from graph expecpt the greates component.
     * @return array of all positions remaining in graph.
     */
    void ReduceToGreatestComponent() {
      std::vector<position_t> cur = GetAllVisited(nodes_.begin()->first);
      std::vector<position_t> next;
      while(true) {
        // Get new component
        position_t new_pos = GetPosNotInComponent(cur);
        if (new_pos.first == -1 && new_pos.second == -1)
          break;
        next = GetAllVisited(new_pos);
        // Determine greater component: swap next and cur if next is greater than cur.
        if (next.size() > cur.size()) {
          auto swap = cur;
          cur = next;
          next = cur;
        }
        // Erase all nodes in smaller component
        for (auto it : next) {
          delete nodes_[it];
          nodes_.erase(it);
        }
      }
    }

    std::vector<position_t> GetAllVisited(position_t pos) {
      // Initialize all nodes as not-vistited.
      std::map<position_t, bool> visited; 
      for (auto node : nodes_) {
        position_t cur_pos = {node.second->line_, node.second->col_};
        visited[cur_pos] = cur_pos == pos;  // initialize all with false expecpt given position.
      }
      // Get all nodes which can be visited from player-den.
      std::list<Node*> queue = {nodes_.at(pos)};
      while(!queue.empty()) {
        auto cur = queue.front();
        queue.pop_front();
        for (auto node : cur->nodes_) {
          position_t pos = {node->line_, node->col_};
          if (!visited[pos]) {
            visited[pos] = true;
            queue.push_back(node);
          }
        }
      }
      std::vector<position_t> vistited_positions;
      for (auto it : visited) {
        if (it.second)
          vistited_positions.push_back(it.first);
      }
      return vistited_positions;
    }

    std::list<position_t> FindWay(position_t pos_a, position_t pos_b) const {
      // Initialize all nodes as not-visited.
      std::map<position_t, position_t> visited; 
      for (auto node : nodes_) {
        position_t pos = {node.second->line_, node.second->col_};
        visited[pos] = {-1, -1};
      }
      // Get all nodes which can be visited from player-den.
      std::list<Node*> queue;
      visited[pos_a] = {pos_a.first, pos_a.second};
      queue.push_back(nodes_.at(pos_a));
      while(!queue.empty()) {
        auto cur = queue.front();
        queue.pop_front();
        // Check if desired node was found.
        if (pos_b.first == cur->line_ && pos_b.second == cur->col_)
          break;
        // iterate over children.
        for (auto node : cur->nodes_) {
          position_t pos = {node->line_, node->col_};
          if (visited[pos] == std::make_pair(-1, -1)) {
            visited[pos] = {cur->line_, cur->col_};
            queue.push_back(node);
          }
        }
      }
      if (visited[pos_b] == std::make_pair(-1, -1))
        throw "Could not find enemy den!.";
      
      std::list<position_t> way = { pos_b };
      while (way.back() != pos_a)
        way.push_back({visited[way.back()].first, visited[way.back()].second});
      //Reverse path and return.
      std::reverse(std::begin(way), std::end(way));
      return way;
    }

  private:
    std::map<position_t, Node*> nodes_;
};

#endif
