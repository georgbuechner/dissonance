#ifndef SRC_DATA_STRUCTS_H_
#define SRC_DATA_STRUCTS_H_

#include <map>
#include <math.h>
#include <string>
#include <vector>

#include "share/objects/units.h"
#include "share/tools/utils/utils.h"

class Resource {
  public:
    Resource(double init, int max, int distributed_iron, bool to_int, position_t pos);

    // getter
    double cur() const;
    double bound() const;
    int limit() const;
    int distributed_iron() const;
    bool blocked() const;
    position_t pos() const;
    double total() const;
    double spent() const;
    double average_boost() const;
    double average_bound() const;
    double average_neg_factor() const;
    double active_percent() const;

    // setter 
    void set_cur(double value);
    void set_bound(double value);
    void set_distributed_iron(int value);
    void set_limit(int value);
    void set_blocked(bool value);

    // functions
    
    /**
     * Checks if resource is active (at least 2 iron distributed).
     * @return whether resource is activated.
     */
    bool Active() const;

    /**
     * Increases current free.
     * Increasesing is done with the following formular: ([boast] * gain * [negative-faktor])/[slowdown]
     * Boast is calculated as: `1 + boast/10` -> in range: [1..2]
     * Negative-faktor is calculated as: `1 - (cur+bound)/max` -> in range: [0..1]
     */
    void Increase(double gain, double slowdown);

    /**
     * Descrease resource.
     * If bind is true, adds amount to bound part of the resource.
     * @param[in] value by which to decrease.
     * @param[in] bind 
     */
    void Decrease(double value, bool bind);

    /**
     * Informs resource that it has been called 'remeber' how often it was
     * called while activated/ not activated.
     */
    void Call();

  private:
    // members
    double free_;  ///< current availible 
    double bound_;  ///< current bound
    int limit_;  ///< maximum of this resource (free+bound)
    int distributed_iron_;
    bool blocked_;
    const bool to_int_;
    const position_t pos_;

    // statistics
    double total_;
    double spent_;
    std::vector<int> average_boost_;
    std::vector<int> average_bound_;
    std::vector<double> average_neg_factor_;
    std::pair<int, int> active_percent_;
};

#endif
