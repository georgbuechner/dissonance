#ifndef SRC_COSTS_H
#define SRC_COSTS_H

#include <map>
#include "codes.h"

namespace costs {
  typedef const std::map<int, double> Costs;

  const std::map<int, Costs> units_costs_ = {
    {UnitsTech::NUCLEUS, {
        {Resources::IRON, 1},
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
        {Resources::IRON, 1}, 
        {Resources::OXYGEN, 0}, 
        {Resources::POTASSIUM, 0}, 
        {Resources::CHLORIDE, 0}, 
        {Resources::GLUTAMATE, 0}, 
        {Resources::DOPAMINE, 17.7}, 
        {Resources::SEROTONIN, 0}
      }
    },
    {UnitsTech::SWARM, {
        {Resources::IRON, 1}, 
        {Resources::OXYGEN, 0}, 
        {Resources::POTASSIUM, 0}, 
        {Resources::CHLORIDE, 0}, 
        {Resources::GLUTAMATE, 0}, 
        {Resources::DOPAMINE, 19.9}, 
        {Resources::SEROTONIN, 0}
      }
    },
    {UnitsTech::TARGET, {
        {Resources::IRON, 1}, 
        {Resources::OXYGEN, 0}, 
        {Resources::POTASSIUM, 0}, 
        {Resources::CHLORIDE, 0}, 
        {Resources::GLUTAMATE, 0}, 
        {Resources::DOPAMINE, 16.5}, 
        {Resources::SEROTONIN, 0}
      }
    },
    {UnitsTech::TOTAL_RESOURCE, {
        {Resources::IRON, 1}, 
        {Resources::OXYGEN, 0}, 
        {Resources::POTASSIUM, 0}, 
        {Resources::CHLORIDE, 0}, 
        {Resources::GLUTAMATE, 0}, 
        {Resources::DOPAMINE, 18.5}, 
        {Resources::SEROTONIN, 17.9}
      }
    },
    {UnitsTech::CURVE, {
        {Resources::IRON, 1}, 
        {Resources::OXYGEN, 0}, 
        {Resources::POTASSIUM, 0}, 
        {Resources::CHLORIDE, 0}, 
        {Resources::GLUTAMATE, 0}, 
        {Resources::DOPAMINE, 21.0}, 
        {Resources::SEROTONIN, 21.2}
      }
    },
    {UnitsTech::ATK_POTENIAL, {
        {Resources::IRON, 1}, 
        {Resources::OXYGEN, 0}, 
        {Resources::POTASSIUM, 10}, 
        {Resources::CHLORIDE, 0}, 
        {Resources::GLUTAMATE, 0}, 
        {Resources::DOPAMINE, 16.0}, 
        {Resources::SEROTONIN, 11.2}
      }
    },
    {UnitsTech::ATK_SPEED, {
        {Resources::IRON, 1}, 
        {Resources::OXYGEN, 0}, 
        {Resources::POTASSIUM, 10}, 
        {Resources::CHLORIDE, 0}, 
        {Resources::GLUTAMATE, 0}, 
        {Resources::DOPAMINE, 19.0}, 
        {Resources::SEROTONIN, 13.2}
      }
    },
    {UnitsTech::ATK_DURATION, {
        {Resources::IRON, 1}, 
        {Resources::OXYGEN, 0}, 
        {Resources::POTASSIUM, 10}, 
        {Resources::CHLORIDE, 0}, 
        {Resources::GLUTAMATE, 0}, 
        {Resources::DOPAMINE, 17.5}, 
        {Resources::SEROTONIN, 12.2}
      }
    },
    {UnitsTech::DEF_SPEED, {
        {Resources::IRON, 1}, 
        {Resources::OXYGEN, 0}, 
        {Resources::POTASSIUM, 0}, 
        {Resources::CHLORIDE, 0}, 
        {Resources::GLUTAMATE, 15.8}, 
        {Resources::DOPAMINE, 16.5}, 
        {Resources::SEROTONIN, 6.6}
      }
    },
    {UnitsTech::DEF_POTENTIAL, {
        {Resources::IRON, 1}, 
        {Resources::OXYGEN, 0}, 
        {Resources::POTASSIUM, 0}, 
        {Resources::CHLORIDE, 0}, 
        {Resources::GLUTAMATE, 15.9}, 
        {Resources::DOPAMINE, 14.5}, 
        {Resources::SEROTONIN, 17.6}
      }
    },
    {UnitsTech::NUCLEUS_RANGE, {
        {Resources::IRON, 1}, 
        {Resources::OXYGEN, 10}, 
        {Resources::POTASSIUM, 0}, 
        {Resources::CHLORIDE, 0}, 
        {Resources::GLUTAMATE, 0}, 
        {Resources::DOPAMINE, 13.5}, 
        {Resources::SEROTONIN, 17.9}
      }
    }
  };
}

#endif
