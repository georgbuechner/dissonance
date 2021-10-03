#ifndef SRC_DATA_STRUCTS_H_
#define SRC_DATA_STRUCTS_H_

#include <map>
#include <math.h>
#include <string>
#include <vector>
#include "spdlog/spdlog.h"
#include "utils/utils.h"

#define LOGGER "logger"

class Resource {
  public:
    Resource(double init, unsigned int max, int distributed_iron, bool to_int) : to_int_(to_int) {
      free_ = init;
      limit_ = max;
      bound_ = 0;
      distributed_iron_ = distributed_iron;
      blocked_ = false;
    }

    // getter
    double cur() const {
      return (to_int_) ? (int)free_ : free_;
    }
    double bound() const {
      return bound_;
    }
    unsigned int limit() const {
      return limit_;
    }
    unsigned int distributed_iron() const {
      return distributed_iron_;
    }

    // setter 
    void set_cur(double value) {
      free_ = value;
    }
    void set_bound(double value) {
      bound_ = value;
    }
    void set_distribited_iron(unsigned int value) {
      distributed_iron_ = value;
    }
    void set_limit(unsigned int value) {
      limit_ = value;
    }

    // functions
   
    /**
     * Checks if resource is active (at least 2 iron distributed).
     * @return whether resource is activated.
     */
    bool Active() const {
      return distributed_iron_ >= 2;
    }

    /**
     * Gets string representation of resource.
     * @return resource as string in format: [cur]+[bound]/[max]
     */
    std::string Print() const {
      return utils::dtos(cur()) + "+" + utils::dtos(bound_) + "/" + utils::dtos(limit_);
    }

    /**
     * Increases current free.
     * Increasesing is done with the following formular: ([boast] * gain * [negative-faktor])/[slowdown]
     * Boast is calculated as: `1 + boast/10` -> in range: [1..2]
     * Negative-faktor is calculated as: `1 - (cur+bound)/max` -> in range: [0..1]
     * TODO (fux): what to do with slowdown?
     */
    void IncreaseResource(double gain, int slowdown) {
      auto calc_boast = [](int boast) -> double { return 1+static_cast<double>(boast)/10; };
      auto calc_negative_factor = [](double cur, double max) -> double { return 1-cur/max; };
      double val = (calc_boast(distributed_iron_) * ((false) ? 1 : gain) * calc_negative_factor(free_+bound_, limit_))/slowdown;
      if (val < 0)
        spdlog::get(LOGGER)->error("Increasing by neg value!! boast: {} gain: {} neg: {}, others {}, {}, {}", 
            calc_boast(distributed_iron_), gain, calc_negative_factor(free_+bound_, limit_), free_, bound_, limit_);
      free_ += val;
    }

  private:
    // members
    double free_;  ///< current availible 
    double bound_;  ///< current bound
    unsigned int limit_;  ///< maximum of this resource (free+bound)
    unsigned int distributed_iron_;
    bool blocked_;
    const bool to_int_;
};

#endif
