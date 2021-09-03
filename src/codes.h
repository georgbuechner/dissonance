#ifndef SRC_CODES_H_
#define SRC_CODES_H_

#include <map>
#include <string>

#define SYMBOL_HILL " "
#define SYMBOL_DEN "\u1455B"
#define SYMBOL_IRON "\u03B6" // alchemisches symbol für Eisenerz 
#define SYMBOL_OXYGEN "\u03B8" // alchemisches symbol für Eisenerz 
#define SYMBOL_POTASSIUM "\u03BA" // alchemisches symbol für kaliumtartrat
#define SYMBOL_CHLORIDE "\u03B3" // alchemisches symbol für Ammoniumchlorid
#define SYMBOL_GLUTAMATE "\u03B7" // alchemisches symbol für essig  (glutamate acid -> acid -> essig)
#define SYMBOL_DOPAMINE "\u03B4" // alchemisches symbol für atimon-erz
#define SYMBOL_SEROTONIN "\u03C3" // alchemisches symbol für atimon
#define SYMBOL_FREE "\u22C5" // simple dot.
#define SYMBOL_DEF "\u0398"
#define SYMBOL_BARACK "\u0394"
#define SYMBOL_INFINITY "\u221E"


enum ViewRange {
  HIDE = 0,
  GRAPH = 1,
  RANGE = 2,
};

enum UnitsTech {
  ACTIVATEDNEURON = 0,
  SYNAPSE,
  EPSP,
  IPSP,
  NUCLEUS, 

  WAY,
  SWARM,
  TARGET,
  TOTAL_OXYGEN, 
  TOTAL_RESOURCE, 
  CURVE,
  ATK_POTENIAL,
  ATK_SPEED,
  ATK_DURATION,
  DEF_POTENTIAL,
  DEF_SPEED,
  NUCLEUS_RANGE,
};

enum Resources {
  IRON = 0,
  OXYGEN,
  POTASSIUM,
  CHLORIDE,
  GLUTAMATE,
  DOPAMINE,
  SEROTONIN,
};

const std::map<int, std::string> resources_name_mapping = {
  {IRON, "iron"},
  {OXYGEN, "oxygen"},
  {POTASSIUM, "potassium"},
  {CHLORIDE, "chloride"},
  {GLUTAMATE, "glutamate"},
  {DOPAMINE, "dopamine"},
  {SEROTONIN, "serotonin"},
  {NUCLEUS, "nucleus"},
};

const std::map<std::string, int> resources_symbol_mapping = {
  {SYMBOL_IRON, IRON},
  {SYMBOL_OXYGEN, OXYGEN},
  {SYMBOL_POTASSIUM, POTASSIUM},
  {SYMBOL_CHLORIDE, CHLORIDE},
  {SYMBOL_GLUTAMATE, GLUTAMATE},
  {SYMBOL_DOPAMINE, DOPAMINE},
  {SYMBOL_SEROTONIN, SEROTONIN},
};

const std::map<int, std::string> units_tech_mapping = {
  {ACTIVATEDNEURON, "activated neuron"},
  {SYNAPSE, "synapse"},
  {EPSP, "epsp"},
  {IPSP, "ipsp"},
  {NUCLEUS, "nucleus"},
  {WAY,"choose way"},
  {SWARM,"swarm attacks"},
  {TARGET,"choose target"},
  {TOTAL_OXYGEN, "Increase total oxygen"},
  {TOTAL_RESOURCE, "Increase total of one rersource"},
  {CURVE, "lower rersource curve slowdown"},
  {ATK_POTENIAL, "Increase potential"},
  {ATK_SPEED, "Increase speed of potentials"},
  {ATK_DURATION, "Increase ipsp duration"},
  {DEF_POTENTIAL, "Increase potential of activated neuron"},
  {DEF_SPEED, "Increase cooldown of activated neuron"},
  {NUCLEUS_RANGE, "Increase cooldown of activated neuron"},
};

#endif
