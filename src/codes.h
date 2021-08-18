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


enum ViewRange {
  HIDE = 0,
  GRAPH = 1,
  RANGE = 2,
};

enum Units {
  ACTIVATEDNEURON = 0,
  SYNAPSE,
  EPSP,
  IPSP,
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


#endif
