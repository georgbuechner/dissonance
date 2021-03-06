#ifndef SRC_CODES_H_
#define SRC_CODES_H_

#include <map>
#include <string>

#define SYMBOL_HILL " "
#define SYMBOL_DEN "\u03A7" // greek uppercase 'chi'
#define SYMBOL_DEF "\u03A6"  // greek uppercase 'phi'
#define SYMBOL_BARACK "\u039E" // greek uppercase 'xi'

#define SYMBOL_IRON "\u03B6" // greek lowercase 'zeta' (from chemical symbol [F]e)
#define SYMBOL_OXYGEN "\u03BF" // greek lowercase 'omicron'
#define SYMBOL_POTASSIUM "\u03BA" // greek lowercase 'kappa' 
#define SYMBOL_CHLORIDE "\u03B3" // greek lowercase 'gamma'
#define SYMBOL_GLUTAMATE "\u03B7" // greek lowercase 'eta'
#define SYMBOL_DOPAMINE "\u03B4" // greek lowercase 'delta'
#define SYMBOL_SEROTONIN "\u03C3" // greek lowercase 'sigma
#define SYMBOL_FREE "\u2219" // simple dot.
#define SYMBOL_INFINITY "\u221E"


enum ViewRange {
  HIDE = 0,
  GRAPH = 1,
  RANGE = 2,
};

enum UnitsTech {
  ACTIVATEDNEURON = 0,
  SYNAPSE,
  NUCLEUS, 
  RESOURCENEURON,
  EPSP,
  IPSP,

  WAY,
  SWARM,
  TARGET,
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

enum Tactics {
  EPSP_FOCUSED = 0,
  IPSP_FOCUSED,
  AIM_NUCLEUS,
  DESTROY_ACTIVATED_NEURONS,
  DESTROY_SYNAPSES,
  BLOCK_ACTIVATED_NEURON,
  BLOCK_SYNAPSES,
  DEF_FRONT_FOCUS,
  DEF_SURROUNG_FOCUS,
  DEF_IPSP_BLOCK,
};

enum Signitue {
  UNSIGNED = 0,
  SHARP = 1,
  FLAT = 0,
};

enum AudioInterval {
  PERFECT_UNISON,
  MINOR_SECOND,
  MAJOR_SECOND,
  MINOR_THIRD,
  MAJOR_THIRD,
  PERFECT_FOURTH,
  TRITONE,
  PERFECT_FIFTH,
  MINOR_SIXTH,
  MAJOR_SIXTH,
  MINOR_SEVENTH,
  MAJOR_SEVENTH,
  PERFECT_OCTAVE,
};

const std::map<int, std::string> tactics_mapping = {
  {EPSP_FOCUSED, "EPSP_FOCUSED"},
  {IPSP_FOCUSED,"IPSP_FOCUSED"},
  {AIM_NUCLEUS,"AIM_NUCLEUS"},
  {DESTROY_ACTIVATED_NEURONS,"DESTROY_ACTIVATED_NEURONS"},
  {DESTROY_SYNAPSES,"DESTROY_SYNAPSES"},
  {BLOCK_ACTIVATED_NEURON,"BLOCK_ACTIVATED_NEURON"},
  {BLOCK_SYNAPSES,"BLOCK_SYNAPSES"},
  {DEF_FRONT_FOCUS,"DEF_FRONT_FOCUS"},
  {DEF_SURROUNG_FOCUS,"DEF_SURROUNG_FOCUS"},
  {DEF_IPSP_BLOCK,"DEF_IPSP_BLOCK"}
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

const std::map<int, std::string> units_tech_mapping = {
  {ACTIVATEDNEURON, "activated-neuron"},
  {RESOURCENEURON, "rersource-neuron"},
  {SYNAPSE, "synapse"},
  {EPSP, "epsp"},
  {IPSP, "ipsp"},
  {NUCLEUS, "nucleus"},
  {WAY,"choose way"},
  {SWARM,"swarm attack"},
  {TARGET,"choose target"},
  {TOTAL_RESOURCE, "increase rersource limits"},
  {CURVE, "lower rersource slowdown"},
  {ATK_POTENIAL, "increase epsp/ ipsp potential"},
  {ATK_SPEED, "Increase epsp/ipsp speed"},
  {ATK_DURATION, "increase ipsp duration"},
  {DEF_POTENTIAL, "increase activated-neuron potential"},
  {DEF_SPEED, "increase activated-neuron cooldown"},
  {NUCLEUS_RANGE, "increase range of nucleus"},
};

#endif
