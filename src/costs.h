#ifndef SRC_COSTS_H
#define SRC_COSTS_H

#include <map>
#include "codes.h"

namespace costs {
  typedef const std::map<int, double> Costs;

  const std::map<int, Costs> units_costs_ = {
    {UnitsTech::NUCLEUS, {
        {Resources::OXYGEN, 30},
        {Resources::POTASSIUM, 30}, 
        {Resources::CHLORIDE, 30}, 
        {Resources::GLUTAMATE, 30}, 
        {Resources::DOPAMINE, 30}, 
        {Resources::SEROTONIN, 30}
      }
    },
    {UnitsTech::ACTIVATEDNEURON, {
        {Resources::OXYGEN, 8.9}, 
        {Resources::POTASSIUM, 0}, 
        {Resources::CHLORIDE, 0}, 
        {Resources::GLUTAMATE, 19.1}, 
        {Resources::DOPAMINE, 0}, 
        {Resources::SEROTONIN, 0}
      }
    },
    {UnitsTech::SYNAPSE, {
        {Resources::OXYGEN, 13.4}, 
        {Resources::POTASSIUM, 6.6}, 
        {Resources::CHLORIDE, 0}, 
        {Resources::GLUTAMATE, 0}, 
        {Resources::DOPAMINE, 0}, 
        {Resources::SEROTONIN, 0}
      }
    },
    {UnitsTech::EPSP, {
        {Resources::OXYGEN, 0}, 
        {Resources::POTASSIUM, 4.4}, 
        {Resources::CHLORIDE, 0}, 
        {Resources::GLUTAMATE, 0}, 
        {Resources::DOPAMINE, 0}, 
        {Resources::SEROTONIN, 0}
      }
    },
    {UnitsTech::IPSP, {
        {Resources::OXYGEN, 0}, 
        {Resources::POTASSIUM, 3.4}, 
        {Resources::CHLORIDE, 6.8}, 
        {Resources::GLUTAMATE, 0}, 
        {Resources::DOPAMINE, 0}, 
        {Resources::SEROTONIN, 0}
      }
    },
    {UnitsTech::WAY, {
        {Resources::OXYGEN, 0}, 
        {Resources::POTASSIUM, 0}, 
        {Resources::CHLORIDE, 0}, 
        {Resources::GLUTAMATE, 0}, 
        {Resources::DOPAMINE, 7.7}, 
        {Resources::SEROTONIN, 0}
      }
    },
    {UnitsTech::SWARM, {
        {Resources::OXYGEN, 0}, 
        {Resources::POTASSIUM, 0}, 
        {Resources::CHLORIDE, 0}, 
        {Resources::GLUTAMATE, 0}, 
        {Resources::DOPAMINE, 9.9}, 
        {Resources::SEROTONIN, 0}
      }
    },
    {UnitsTech::TARGET, {
        {Resources::OXYGEN, 0}, 
        {Resources::POTASSIUM, 0}, 
        {Resources::CHLORIDE, 0}, 
        {Resources::GLUTAMATE, 0}, 
        {Resources::DOPAMINE, 6.5}, 
        {Resources::SEROTONIN, 0}
      }
    },
    {UnitsTech::TOTAL_OXYGEN, {
        {Resources::OXYGEN, 0}, 
        {Resources::POTASSIUM, 0}, 
        {Resources::CHLORIDE, 0}, 
        {Resources::GLUTAMATE, 0}, 
        {Resources::DOPAMINE, 7.5}, 
        {Resources::SEROTONIN, 8.9}
      }
    },
    {UnitsTech::TOTAL_RESOURCE, {
        {Resources::OXYGEN, 0}, 
        {Resources::POTASSIUM, 0}, 
        {Resources::CHLORIDE, 0}, 
        {Resources::GLUTAMATE, 0}, 
        {Resources::DOPAMINE, 8.5}, 
        {Resources::SEROTONIN, 7.9}
      }
    },
    {UnitsTech::CURVE, {
        {Resources::OXYGEN, 0}, 
        {Resources::POTASSIUM, 0}, 
        {Resources::CHLORIDE, 0}, 
        {Resources::GLUTAMATE, 0}, 
        {Resources::DOPAMINE, 11.0}, 
        {Resources::SEROTONIN, 11.2}
      }
    },
    {UnitsTech::ATK_POTENIAL, {
        {Resources::OXYGEN, 0}, 
        {Resources::POTASSIUM, 0}, 
        {Resources::CHLORIDE, 0}, 
        {Resources::GLUTAMATE, 0}, 
        {Resources::DOPAMINE, 5.0}, 
        {Resources::SEROTONIN, 11.2}
      }
    },
    {UnitsTech::ATK_SPEED, {
        {Resources::OXYGEN, 0}, 
        {Resources::POTASSIUM, 0}, 
        {Resources::CHLORIDE, 0}, 
        {Resources::GLUTAMATE, 0}, 
        {Resources::DOPAMINE, 3.0}, 
        {Resources::SEROTONIN, 9.2}
      }
    },
    {UnitsTech::ATK_DURATION, {
        {Resources::OXYGEN, 0}, 
        {Resources::POTASSIUM, 0}, 
        {Resources::CHLORIDE, 0}, 
        {Resources::GLUTAMATE, 0}, 
        {Resources::DOPAMINE, 2.5}, 
        {Resources::SEROTONIN, 11.2}
      }
    },
    {UnitsTech::DEF_SPEED, {
        {Resources::OXYGEN, 0}, 
        {Resources::POTASSIUM, 0}, 
        {Resources::CHLORIDE, 0}, 
        {Resources::GLUTAMATE, 5.8}, 
        {Resources::DOPAMINE, 6.5}, 
        {Resources::SEROTONIN, 6.6}
      }
    },
    {UnitsTech::DEF_POTENTIAL, {
        {Resources::OXYGEN, 0}, 
        {Resources::POTASSIUM, 0}, 
        {Resources::CHLORIDE, 0}, 
        {Resources::GLUTAMATE, 5.9}, 
        {Resources::DOPAMINE, 4.5}, 
        {Resources::SEROTONIN, 7.6}
      }
    },
    {UnitsTech::NUCLEUS_RANGE, {
        {Resources::OXYGEN, 0}, 
        {Resources::POTASSIUM, 0}, 
        {Resources::CHLORIDE, 0}, 
        {Resources::GLUTAMATE, 6.9}, 
        {Resources::DOPAMINE, 3.5}, 
        {Resources::SEROTONIN, 7.9}
      }
    }

  };
}

#endif
