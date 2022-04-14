#ifndef SRC_GRAPH_H_
#define SRC_GRAPH_H_

#include "share/defines.h"
#include "share/tools/fiboqueue.h"
#include "share/tools/utils/utils.h"
#include <algorithm>
#include <curses.h>
#include <iostream>
#include <iterator>
#include <mutex>
#include <queue>
#include <shared_mutex>
#include <unordered_map>
#include <utility>
#include <list>
#include <map>
#include <set>
#include <vector>

struct Queue {
  std::multimap<float, position_t> queue_;

  bool empty() {
    return queue_.size() == 0;
  }

  void insert(position_t pos, float priority) {
    queue_.insert(std::make_pair(priority, pos));
  }

  position_t pop_front() {
    position_t pos = queue_.begin()->second;
    queue_.erase(queue_.begin());
    return pos;
  }

  void descrease_key(position_t pos, float old_p, float new_p) {
    auto ret = queue_.equal_range(old_p);
    for (auto& it=ret.first; it!=ret.second; ++it) {
      if (it->second == pos) {
        queue_.erase(it);
        break;
      }
    } 
    insert(pos, new_p);
  }
};

struct QueueF {
  FibQueue<double> fq;

  bool empty() {
    return fq.empty();
  }

  void insert(position_t pos, float priority) {
    fq.push(priority, utils::PositionToString(pos));
  }

  position_t pop_front() {
    return utils::PositionFromString(fq.pop());
  }

  void descrease_key(position_t pos, float old_p, float new_p) {
    fq.decrease_key(utils::PositionToString(pos), old_p, new_p);
  }
};

struct Node {
  position_t pos_;
  std::list<Node*> nodes_; // TODO (fux): concider implementing map[dist] -> Node*. Also do we need Node*?
};

class Graph {
  public:
    typedef std::pair<position_t, position_t> t_s_e_tuple;
    Graph() { }

    // getter:
    const std::map<position_t, Node*>& nodes() const {
      return nodes_; 
    }

    void AddNode(int line, int col) {
      position_t pos = {line, col};
      nodes_[pos] = new Node({{line, col}, {}});
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
      for (auto node : nodes_)
        visited[node.second->pos_] = node.second->pos_ == pos;  // initialize all with false expecpt given position.
      // Get all nodes which can be visited from player-den.
      std::list<Node*> queue = {nodes_.at(pos)};
      while(!queue.empty()) {
        auto cur = queue.front();
        queue.pop_front();
        for (auto node : cur->nodes_) {
          if (!visited[node->pos_]) {
            visited[node->pos_] = true;
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
      for (auto node : nodes_)
        visited[node.second->pos_] = {-1, -1};
      // Get all nodes which can be visited from player-den.
      std::list<Node*> queue;
      visited[pos_a] = pos_a; 
      queue.push_back(nodes_.at(pos_a));
      while(!queue.empty()) {
        auto cur = queue.front();
        queue.pop_front();
        // Check if desired node was found.
        if (pos_b == cur->pos_)
          break;
        // iterate over children.
        for (auto node : cur->nodes_) {
          if (visited[node->pos_] == std::make_pair(-1, -1)) {
            visited[node->pos_] = cur->pos_;
            queue.push_back(node);
          }
        }
      }
      if (visited[pos_b] == std::make_pair(-1, -1))
        throw "Could not find way!.";
      
      std::list<position_t> way = { pos_b };
      while (way.back() != pos_a)
        way.push_back({visited[way.back()].first, visited[way.back()].second});
      //Reverse path and return.
      std::reverse(std::begin(way), std::end(way));
      return way;
    }

    std::list<position_t> DijkstrasWay(position_t s, position_t t) {
      if (s == t) {
        return {s};
      }
      // Check cache
      t_s_e_tuple start_end_tuple = {s, t};
      std::shared_lock sl(mutex_cache_);
      if (chache_.count(start_end_tuple) > 0) {
        return chache_.at(start_end_tuple);
      }
      sl.unlock();

      // Initialize search
      std::unordered_map<std::string, float> distance;
      std::map<position_t, position_t> prev;
      QueueF q;
      // Set distance from each node (v) to start node (s) to (v in s(neighbors)) ? dist(v,s) : 999999
      for (const auto & it : nodes_) {
        std::string pos_string = utils::PositionToString(it.first);
        distance[pos_string] = 9999999;
        q.insert(it.first, 9999999);
      }
      for (const auto& it : nodes_.at(s)->nodes_) {
        std::string pos_string = utils::PositionToString(it->pos_);
        auto dst = utils::Dist(s, it->pos_);
        distance[pos_string] = dst;
        q.descrease_key(it->pos_, 9999999, dst);
      }

      // Do search
      while (!q.empty()) {
        auto w = q.pop_front();
        if (w == t)
          break;
        for (const auto& it : nodes_.at(w)->nodes_) {
          double c = distance[utils::PositionToString(w)] + utils::Dist(w, it->pos_);
          std::string pos_string = utils::PositionToString(it->pos_);
          double old = distance[pos_string];
          if (old > c) {
            q.descrease_key(it->pos_, old, c);
            distance[pos_string] = c;
            prev[it->pos_] = w;
          }
        }
      }
      // Get way
      std::list<position_t> way = {t};
      if (prev.count(t) > 0) {
        while (prev.count(t)>0) {
          way.push_front(prev[t]);
          t = prev[t];
        }
        way.push_front(s);
        // Add way to cache
        std::unique_lock ul(mutex_cache_);
        chache_[start_end_tuple] = way;
        return way;
      }
      throw "Could not find way: " + utils::PositionToString(s) + " to " + utils::PositionToString(t);
    }

  private:
    std::map<position_t, Node*> nodes_;
    std::shared_mutex mutex_cache_;  ///< mutex locked, when accessing cache.
    std::map<t_s_e_tuple, std::list<position_t>> chache_;
};

#endif
