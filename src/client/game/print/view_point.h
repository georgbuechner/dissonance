#ifndef SRC_CLIENT_GAME_PRINT_VIEWPOINT_H_
#define SRC_CLIENT_GAME_PRINT_VIEWPOINT_H_

#include <iostream>
#include <memory>
#include <set>
#include <spdlog/spdlog.h>
#include <string>
#include "share/defines.h"
#include "share/constants/codes.h"
#include "share/constants/costs.h"
#include "share/shemes/data.h"
#include "share/tools/utils/utils.h"

// Selection
class ViewPoint {
  public: 
    ViewPoint() {};
    ViewPoint(int x, int y, void(ViewPoint::*f_inc)(int val), std::string(ViewPoint::*f_ts)()) 
      : x_(x), y_(y), inc_(f_inc), to_string_(f_ts) {
        spdlog::get(LOGGER)->info("Created new ViewPoint with x={} and y={}", x_, y_);
      }

    // getter 
    int x() const { return x_; } 
    int y() const { return y_; } 
    std::pair<position_t, int> range() const { return range_; }

    // setter 
    void set_x(int x) { x_ = x; }
    void set_y(int y) { y_ = y; }
    void set_resources(std::shared_ptr<std::map<int, Data::Resource>> resources) {
      resources_ = resources;
    }
    void set_technologies(std::shared_ptr<std::map<int, tech_of_t>> technologies) {
      technologies_ = technologies;
    }
    void set_graph_positions(std::shared_ptr<std::set<position_t>> graph_positions) { 
      graph_positions_ = graph_positions; 
    }
    void set_range(std::pair<position_t, int> range) { range_ = range; }

    // methods
    void inc(int val) { (this->*inc_)(val); }
    std::string to_string() { return(this->*to_string_)(); }

    void inc_resource(int val) { x_= utils::Mod(x_+val, SEROTONIN+1); }
    void inc_tech(int val) { x_= utils::Mod(x_+val, NUCLEUS_RANGE+1, WAY); }
    void inc_stats(int val) { x_ = utils::Mod(x_+val, y_); }
    void inc_field(int val) { 
      spdlog::get(LOGGER)->debug("ViewPoint::inc_field: Changing field selection: {}|{}", y_, x_);
      int old_x = x_;
      int old_y = y_;

      position_t next_diag;
      if (val == 1) {
        y_++;
        next_diag = {y_, x_+1};
      }
      else if (val == -1) {
        y_--;
        next_diag = {y_, x_-1};
      }
      else if (val == 2) {
        x_++;
        next_diag = {y_-1, x_};
      }
      else if (val == -2) {
        x_--;
        next_diag = {y_+1, x_};
      }

      position_t pos = {y_, x_};
      // If not full field range: check if next position is a) still in range and b) valid graph positions
      if (range_.second < 999 && 
          (utils::Dist(range_.first, pos) > range_.second || graph_positions_->count(pos) == 0)) {
        // If diagonal position possible, do that.
        if (graph_positions_->count(next_diag) > 0 && utils::Dist(range_.first, next_diag) <= range_.second) {
          y_ = next_diag.first;
          x_ = next_diag.second;
        }
        // Otherwise, revert changes.
        else {
          x_ = old_x;
          y_ = old_y;
        }
      }
    }

    std::string to_string_resource() {   
      auto resource = resources_->at(x_);
      return resources_name_mapping.at(x_) + ": " + resource.value_ + "+" 
        + resource.bound_ + "/" + resource.limit_ + " ++" + resource.iron_
        + "$" + resource_description_mapping.at(x_);
    }

    std::string to_string_tech() {
      auto tech = technologies_->at(x_);
      std::string costs = "costs: ";
      for (const auto& it : costs::units_costs_.at(x_)) {
        if (it.second > 0)
          costs += resources_name_mapping.at(it.first) + ": " + utils::Dtos(it.second) + ", ";
      }
      return units_tech_name_mapping.at(x_) + ": " + std::to_string(tech.first) + "/" 
        + std::to_string(tech.second) + "$" + units_tech_description_mapping.at(x_) + "$" + costs;
    }

    std::string to_string_field() {
      return std::to_string(x_) + "|" + std::to_string(y_);
    }

  private:
    int x_;
    int y_;
    std::pair<position_t, int> range_; // start, range
    std::shared_ptr<std::map<int, Data::Resource>> resources_;
    std::shared_ptr<std::map<int, tech_of_t>> technologies_;
    std::shared_ptr<std::set<position_t>> graph_positions_;
    void(ViewPoint::*inc_)(int val);
    std::string(ViewPoint::*to_string_)();
};

#endif
