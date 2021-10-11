#ifndef SRC_DATA_STRUCTS_H_
#define SRC_DATA_STRUCTS_H_

#include <map>
#include <math.h>
#include <string>
#include <vector>
#include "objects/units.h"
#include "spdlog/spdlog.h"
#include "utils/utils.h"

#define LOGGER "logger"

class Resource {
  public:
    Resource(double init, unsigned int max, int distributed_iron, bool to_int, position_t pos);

    // getter
    double cur() const;
    double bound() const;
    unsigned int limit() const;
    unsigned int distributed_iron() const;
    bool blocked() const;
    position_t pos() const;

    // setter 
    void set_cur(double value);
    void set_bound(double value);
    void set_distribited_iron(unsigned int value);
    void set_limit(unsigned int value);
    void set_blocked(bool value);

    // functions
   
    /**
     * Checks if resource is active (at least 2 iron distributed).
     * @return whether resource is activated.
     */
    bool Active() const;

    /**
     * Gets string representation of resource.
     * @return resource as string in format: [cur]+[bound]/[max]
     */
    std::string Print() const;

    /**
     * Increases current free.
     * Increasesing is done with the following formular: ([boast] * gain * [negative-faktor])/[slowdown]
     * Boast is calculated as: `1 + boast/10` -> in range: [1..2]
     * Negative-faktor is calculated as: `1 - (cur+bound)/max` -> in range: [0..1]
     * TODO (fux): what to do with slowdown?
     */
    void IncreaseResource(double gain, double slowdown);

  private:
    // members
    double free_;  ///< current availible 
    double bound_;  ///< current bound
    unsigned int limit_;  ///< maximum of this resource (free+bound)
    unsigned int distributed_iron_;
    bool blocked_;
    const bool to_int_;
    const position_t pos_;
};

#endif
