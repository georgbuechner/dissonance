#ifndef SRC_GRAPH_H_
#define SRC_GRAPH_H_

#include "share/defines.h"
#include "share/tools/fiboqueue.h"
#include "share/tools/utils/utils.h"
#include "spdlog/spdlog.h"
#include <algorithm>
#include <iostream>
#include <iterator>
#include <mutex>
#include <queue>
#include <random>
#include <shared_mutex>
#include <sys/types.h>
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

  void insert(int pos, float priority) {
    fq.push(priority, pos);
  }

  int pop_front() {
    return fq.pop();
  }

  void descrease_key(int pos, float old_p, float new_p) {
    fq.decrease_key(pos, old_p, new_p);
  }
};

struct Node {
  int i_pos_;
  position_t pos_;
  std::list<std::pair<Node*, double>> nodes_; // TODO (fux): concider implementing map[dist] -> Node*. 
                                              // Also do we need Node*?
};

class Graph {
  public:
    typedef std::pair<position_t, position_t> t_s_e_tuple;
    Graph() { }

    // getter:
    const std::unordered_map<int, Node*>& nodes() const {
      return nodes_; 
    }

    Node* GetNode(position_t pos) const {
      return nodes_.at(to_int(pos));
    }

    void AddNode(int line, int col) {
      position_t pos = {line, col};
      int i_pos = to_int(pos);
      nodes_[i_pos] = new Node({i_pos, pos, {}});
    };

    void AddEdge(Node* a, Node* b) {
      a->nodes_.push_back({b, utils::Dist(a->pos_, b->pos_)});
    }

    bool InGraph(position_t pos) const {
      return nodes_.count(to_int(pos)) > 0;
    }

    int GetPosNotInComponent(std::set<int> component) {
      for (const auto& it : nodes_) {
        if (component.count(it.first) == 0)
          return it.second->i_pos_;
      }
      return -1;
    }

    /**
     * Removes all components from graph expecpt the greates component.
     * @return array of all positions remaining in graph.
     */
    void ReduceToGreatestComponent() {
      std::set<int> cur = GetAllVisited(nodes_.begin()->first);
      std::set<int> next;
      while(true) { 
        // Get new component
        int new_pos = GetPosNotInComponent(cur);
        if (new_pos == -1) 
          break;
        next = GetAllVisited(new_pos);
        // Determine greater component: swap next and cur if next is greater than cur.
        if (next.size() > cur.size()) {
          auto swap = cur;
          cur = next;
          next = swap;
        }
        // Erase all nodes in smaller component
        spdlog::get(LOGGER)->debug("Graph::ReduceToGreatest. Component erased with {} elements", next.size());
        for (auto it : next) {
          delete nodes_[it];
          nodes_.erase(it);
        }
      }
    }

    std::set<int> GetAllVisited(int pos) {
      // Initialize all nodes as not-vistited.
      std::unordered_map<int, bool> visited; 
      for (auto node : nodes_)
        visited[node.first] = node.first == pos;  // initialize all with false except given position.
      // Get all nodes which can be visited from player-den.
      std::queue<Node*> queue;
      queue.push(nodes_.at(pos));
      while(!queue.empty()) {
        auto cur = queue.front();
        queue.pop();
        for (auto node : cur->nodes_) {
          if (!visited[node.first->i_pos_]) {
            visited[node.first->i_pos_] = true;
            queue.push(node.first);
          }
        }
      }
      std::set<int> vistited_positions;
      for (auto it : visited) {
        if (it.second)
          vistited_positions.insert(it.first);
      }
      return vistited_positions;
    }

    std::list<position_t> FindWay(position_t pos_a, position_t pos_b) const {
      int i_pos_a = to_int(pos_a);
      int i_pos_b = to_int(pos_b);
      // Iialize all nodes as not-visited.
      std::map<int, int> visited; 
      for (auto node : nodes_)
        visited[node.second->i_pos_] = -1;
      // Get all nodes which can be visited from player-den.
      std::list<Node*> queue;
      visited[i_pos_a] = i_pos_a; 
      queue.push_back(nodes_.at(i_pos_a));
      while(!queue.empty()) {
        auto cur = queue.front();
        queue.pop_front();
        // Check if desired node was found.
        if (pos_b == cur->pos_)
          break;
        // iterate over children.
        for (auto node : cur->nodes_) {
          if (visited[node.first->i_pos_] == -1) {
            visited[node.first->i_pos_] = cur->i_pos_;
            queue.push_back(node.first);
          }
        }
      }
      if (visited[i_pos_b] == -1)
        throw "Could not find way!.";
      
      std::list<int> i_way = { i_pos_b };
      while (i_way.back() != i_pos_a)
        i_way.push_back(visited[i_way.back()]);
      //Reverse path and return.
      std::reverse(std::begin(i_way), std::end(i_way));
      std::list<position_t> way;
      for (const auto& it : i_way) 
        way.push_back(get_pos(it));
      return way;
    }

    std::list<position_t> DijkstrasWay(position_t s, position_t t) {
      if (s == t)
        return {s};
      int i_pos_a = to_int(s);
      int i_pos_b = to_int(t);

      // Check cache
      t_s_e_tuple start_end_tuple = {s, t};
      std::shared_lock sl(mutex_cache_);
      if (chache_.count(start_end_tuple) > 0)
        return chache_.at(start_end_tuple);
      t_s_e_tuple end_start_tuple = {t, s};
      // If other direction is in cache, returned reversed.
      if (chache_.count(end_start_tuple) > 0) {
        auto way = chache_.at(end_start_tuple);
        way.reverse();
        return way;
      }
      sl.unlock();

      // Initialize search
      std::unordered_map<int, float> distance;
      std::map<position_t, position_t> prev;
      QueueF q;
      // Set distance from each node (v) to start node (s) to (v in s(neighbors)) ? dist(v,s) : 999999
      for (const auto & it : nodes_) {
        distance[it.first] = 9999999;
        q.insert(it.first, 9999999);
      }
      for (const auto& it : nodes_.at(i_pos_a)->nodes_) {
        // If direct child is target, return {[s]tart, [t]arget} and add to cache.
        if (it.first->pos_ == t) {
          chache_[start_end_tuple] = {s, t};
          return {s, t};
        }
        distance[it.first->i_pos_] = it.second;
        q.descrease_key(it.first->i_pos_, 9999999, it.second);
      }

      // Do search
      while (!q.empty()) {
        auto w = q.pop_front();
        if (w == i_pos_b)
          break;
        for (const auto& it : nodes_.at(w)->nodes_) {
          double c = distance[w] + it.second;
          double old = distance[it.first->i_pos_];
          if (old > c) {
            q.descrease_key(it.first->i_pos_, old, c);
            distance[it.first->i_pos_] = c;
            prev[it.first->pos_] = get_pos(w);
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
        std::vector<position_t> way_vec{std::begin(way), std::end(way)};
        for (unsigned int i=0;i<way_vec.size(); i++) {
          t_s_e_tuple part_tuple = {way_vec[i], start_end_tuple.second};
          std::list<position_t> way_part(way_vec.begin()+i, way_vec.end());
          chache_[part_tuple] = way_part;
        }
        return way;
      }
      spdlog::get(LOGGER)->debug("Could not find way: {} to {}", utils::PositionToString(s), utils::PositionToString(t));
      throw "Could not find way: " + utils::PositionToString(s) + " to " + utils::PositionToString(t);
    }

    static int to_int(position_t pos) {
      return pos.first<<16 | pos.second;
    }

    static position_t get_pos(int c) {
      return {c>>16, c & 0xFFFF};
    }

    std::map<t_s_e_tuple, std::list<position_t>> chache() { return chache_; }

  private:
    std::unordered_map<int, Node*> nodes_;
    std::shared_mutex mutex_cache_;  ///< mutex locked, when accessing cache.
    std::map<t_s_e_tuple, std::list<position_t>> chache_;
};

#endif
