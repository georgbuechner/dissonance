#ifndef SRC_CODES_H_
#define SRC_CODES_H_

#include <map>
#include <string>
#include <vector>

#define SYMBOL_HILL " "
#define SYMBOL_DEN "\u03A7" // greek uppercase 'chi'
#define SYMBOL_DEF "\u03A6"  // greek uppercase 'phi'
#define SYMBOL_BARACK "\u039E" // greek uppercase 'xi'
#define SYMBOL_LOOPHOLE "\u221E" // infinity

#define SYMBOL_IRON "\u03B6" // greek lowercase 'zeta' (from chemical symbol [F]e)
#define SYMBOL_OXYGEN "\u03BF" // greek lowercase 'omicron'
#define SYMBOL_POTASSIUM "\u03BA" // greek lowercase 'kappa' 
#define SYMBOL_CHLORIDE "\u03B3" // greek lowercase 'gamma'
#define SYMBOL_GLUTAMATE "\u03B7" // greek lowercase 'eta'
#define SYMBOL_DOPAMINE "\u03B4" // greek lowercase 'delta'
#define SYMBOL_SEROTONIN "\u03C3" // greek lowercase 'sigma
#define SYMBOL_FREE "\u2219" // simple dot.

enum Markers {
  SYNAPSE_MARKER,
  TARGETS_MARKER,
  WAY_MARKER,
  PICK_MARKER,
};

enum Positions {
  PLAYER = 0, 
  ENEMY,
  CENTER,
  TARGETS,
  CURRENT_WAY,
  CURRENT_WAY_POINTS,
};

enum GameStatus {
  WAITING = 0,
  WAITING_FOR_PLAYERS,
  SETTING_UP,
  RUNNING,
  CLOSING,
  CLOSED
};

enum Mode {
  SINGLE_PLAYER = 0,
  MULTI_PLAYER,
  MULTI_PLAYER_CLIENT,
  OBSERVER,
  TUTORIAL,
  SETTINGS,
  AI_GAME
};

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
  LOOPHOLE,
  EPSP,
  IPSP,
  MACRO,

  WAY,
  SWARM,
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
  EPSP_FOCUSED = 0, // AudioKi::ipsp_epsp_strategies_
  IPSP_FOCUSED,

  AIM_NUCLEUS,  // AudioKi::epsp_target_strategies_
  DESTROY_ACTIVATED_NEURONS,
  DESTROY_SYNAPSES,
  DESTROY_RESOURCES,

  BLOCK_ACTIVATED_NEURON, // AudioKi::ipsp_target_strategies_
  BLOCK_SYNAPSES,
  BLOCK_RESOURCES,

  DEF_FRONT_FOCUS, // AudioKi::activated_neuron_strategies_
  DEF_SURROUNG_FOCUS,

  DEF_IPSP_BLOCK, // AudioKi::def_strategies_
  DEF_AN_BLOCK,
};

enum Signitue {
  UNSIGNED = 0,
  SHARP = 1,
  FLAT = 2,
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
  {EPSP_FOCUSED, "EPSP FOCUSED"},
  {IPSP_FOCUSED,"IPSP FOCUSED"},
  {AIM_NUCLEUS,"AIM NUCLEUS"},
  {DESTROY_ACTIVATED_NEURONS,"DESTROY ACTIVATED NEURONS"},
  {DESTROY_SYNAPSES,"DESTROY_SYNAPSES"},
  {DESTROY_RESOURCES,"DESTROY RESOURCES"},
  {BLOCK_ACTIVATED_NEURON,"BLOCK ACTIVATED NEURON"},
  {BLOCK_SYNAPSES,"BLOCK SYNAPSES"},
  {BLOCK_RESOURCES,"BLOCK RESOURCES"},
  {DEF_FRONT_FOCUS,"DEF FRONT FOCUS"},
  {DEF_SURROUNG_FOCUS,"DEF SURROUNG FOCUS"},
  {DEF_IPSP_BLOCK,"DEF IPSP BLOCK"},
  {DEF_AN_BLOCK, "DEF ACTIVATED NEURON BLOCK"}
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

const std::map<std::string, int> symbol_resource_mapping = {
  {SYMBOL_IRON, IRON},
  {SYMBOL_OXYGEN, OXYGEN},
  {SYMBOL_POTASSIUM, POTASSIUM},
  {SYMBOL_CHLORIDE, CHLORIDE},
  {SYMBOL_GLUTAMATE, GLUTAMATE},
  {SYMBOL_DOPAMINE, DOPAMINE},
  {SYMBOL_SEROTONIN, SEROTONIN},
};

const std::map<int, std::string> resource_symbol_mapping = {
  {IRON, SYMBOL_IRON},
  {OXYGEN, SYMBOL_OXYGEN},
  {POTASSIUM, SYMBOL_POTASSIUM},
  {CHLORIDE, SYMBOL_CHLORIDE},
  {GLUTAMATE, SYMBOL_GLUTAMATE},
  {DOPAMINE, SYMBOL_DOPAMINE},
  {SEROTONIN, SYMBOL_SEROTONIN},
};

const std::map<int, std::string> unit_symbol_mapping = {
  {ACTIVATEDNEURON, SYMBOL_DEF},
  {SYNAPSE, SYMBOL_BARACK},
  {NUCLEUS, SYMBOL_DEN},
  {LOOPHOLE, SYMBOL_LOOPHOLE},
};

const std::map<int, std::string> resource_description_mapping = {
  {IRON, "Used for technologies and for boosting resource-gain"},
  {OXYGEN,"Used for creating neurons, also influences resource-gain"},
  {POTASSIUM,"Used for creating potential (ipsp/ epsp) and synapses"},
  {CHLORIDE,"Used for creating ipsp"},
  {GLUTAMATE,"Used for creating activated neurons"},
  {DOPAMINE,"Used for technologies concerning potentials and neurons"},
  {SEROTONIN,"Used for technologies concerning resources"}
};

const std::map<int, std::string> units_tech_name_mapping = {
  {ACTIVATEDNEURON, "activated-neuron"},
  {RESOURCENEURON, "resource-neuron"},
  {LOOPHOLE, "loophole"},
  {SYNAPSE, "synapse"},
  {EPSP, "epsp"},
  {IPSP, "ipsp"},
  {MACRO, "macro"},
  {NUCLEUS, "nucleus"},
  {WAY,"choose way"},
  {SWARM,"swarm attack"},
  {TOTAL_RESOURCE, "resource limits++"},
  {CURVE, "resource slowdown--"},
  {ATK_POTENIAL, "epsp/ ipsp potential++"},
  {ATK_SPEED, "epsp/ipsp speed++"},
  {ATK_DURATION, "ipsp duration++"},
  {DEF_POTENTIAL, "activated-neuron potential++"},
  {DEF_SPEED, "activated-neuron cooldown++"},
  {NUCLEUS_RANGE, "range of nucleus++"},
};

const std::map<int, std::string> units_tech_description_mapping = {
  {ACTIVATEDNEURON, "activated-neuron"},
  {LOOPHOLE, "loophole"},
  {RESOURCENEURON, "resource-neuron"},
  {SYNAPSE, "synapse"},
  {EPSP, "epsp"},
  {IPSP, "ipsp"},
  {MACRO, "macro"},
  {NUCLEUS, "nucleus"},
  {WAY,"Increases maximum number of way-points you may set to specify way of potential (applicable per synapse)"},
  {SWARM,"Activates using swarm-attack/ increases maximum swarm-size possible (applicable per synapse)"},
  {TOTAL_RESOURCE, "Increases resource limits by 10%"},
  {CURVE, "Lowers resource slowdown"},
  {ATK_POTENIAL, "Increases epsp/ ipsp potential"},
  {ATK_SPEED, "Increases epsp/ipsp speed"},
  {ATK_DURATION, "Increases ipsp duration at target"},
  {DEF_POTENTIAL, "Increase activated-neuron's potential"},
  {DEF_SPEED, "Increases activated-neurons cooldown between activation"},
  {NUCLEUS_RANGE, "Increases range of nucleus."},
};

const std::map<char, int> char_unit_mapping = { 
  {'A', ACTIVATEDNEURON}, {'S', SYNAPSE}, {'N', NUCLEUS},
  {'e', EPSP}, {'i', IPSP}, {'m', MACRO}

};

const std::vector<std::string> resource_symbols = {SYMBOL_OXYGEN, SYMBOL_POTASSIUM, SYMBOL_SEROTONIN, SYMBOL_GLUTAMATE, 
    SYMBOL_DOPAMINE, SYMBOL_CHLORIDE};

#endif
