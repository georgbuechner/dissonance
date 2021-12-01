#ifndef SRC_CONSTANTS_TEXTS_H_
#define SRC_CONSTANTS_TEXTS_H_

#include <vector>
#include <string>

namespace texts {

  typedef std::vector<std::string> paragraph_t;
  typedef std::vector<paragraph_t> paragraphs_t;
  const paragraphs_t welcome = {
    {
      "Welcome to DISSONANCE",
      "",
      "This is a beta-version. Please report issues at:",
      "https://github.com/georgbuechner/dissonance/issues"
    },
    {
      "Welcome to DISSONANCE",
      "",
      "Several parts of the brain are in dissonance and started attacking each other",
      "with strong potentials, aiming to destroy the others nucleus."
    },
    {
      "Welcome to DISSONANCE",
      "",
      "Each player starts with a nucleus with control over a few cells surrounding it.",
      "By gathering different resources (iron, oxygen, potassium, ...) you can create synapses to",
      "generate potential, advancing towards the enemies nucleus. By activating cells",
      "you control, these cells can neutralize incoming potential.",
      "",
      "You randomly gain iron every few seconds. Iron can be used to activate the process of",
      "gathering new resources or to boost their production. But your resource production i",
      "also dependent on your current oxygen level and on how close the resource is to its",
      "limit. So be careful when building neurons: the more resources are bound i",
      "neurons, the less resources you gain",
      "",
      "Once you started gaining dopamine and serotonin, you can develop advanced",
      "technologies, allowing you to f.e. increase your resource limits or target specific enemy",
      "neurons and hence destroy enemy synapses or activated neurons or even block the",
      "enemies resource-production.",
      "",
      "Once you gained enough resources you can expand your control over the brain by",
      "building more nucleus'. Use these, your build potential and the strategist",
      "inside you to overcome dissonance in your favor!"
    },
    {
      "When dissonance starts, remember you should boost oxygen and activate production of glutamate, to start defending yourself.",
      "Also keep in mind, that there are two kinds of potential: ",
      "EPSP (strong in attack) and IPSP (blocks buildings); you should start with EPSP."
    }
  };

  const paragraphs_t help = {
    {
      "##### HELP #####",
      "",
      "--- COSTS (potentials/ neurons) ----", 
      "",
      "ACTIVATE NEURON: oxygen=8.9, glutamate=19.1",
      "SYNAPSE: oxygen=13.4, potassium=6.6",
      "EPSP: potassium=4.4",
      "IPSP: potassium=3.4, chloride=6.8",
    },
    {
      "##### HELP #####",
      "",
      "--- COSTS (technologies) ----", 
      "", 
      "attack strategies:",
      "WAY (select way/ way-points for neurons): iron=1, dopamine=17.7",
      "SWARM (launch swarm attacks +3): iron=1, dopamine=19.9",
      "TARGET (choose target: ipsp/ epsp): iron=1, dopamine=16.5",
      "",
      "resources:",
      "TOTAL RESOURCE (max allowed resources: each): iron=1, dopamine=18.5, serotonin=17.9",
      "SLOWDOWN (resource curve slowdown): iron=1, dopamine=21.0, serotonin=21.2",
      "",
      "potential-upgrade:",
      "POTENIAL (increases potential of ipsp/ epsp): iron=1, potassium=10.0, dopamine=16.0, serotonin=11.2",
      "SPEED (increases speed of ipsp/ epsp): iron=1, potassium=10.0, dopamine=19.0 serotonin=13.2",
      "DURATION (increases duration of ipsp): iron=1, potassium=10.0, dopamine=17.5, serotonin=12.2",
      "",
      "activated-neuron upgrade",
      "POTENIAL (increases potential of activated-neuron): iron=1, glutamate=15.9, dopamine=14.5, serotonin=17.6",
      "SPEED (increases speed of activated-neuron): iron=1, glutamate=15.9, dopamine=16.5, serotonin=6.6",
      "",
      "expansion:",
      "RANGE (increases range of nucleus): iron=1, dopamine=13.5, serotonin=17.9",
    },
    {
      "##### HELP #####",
      "",
      "--- TIPS ----", 
      "Iron is used for (boosting) resource production. At least 2 iron must be distributed to a resource, in order for production of this resource to start.",
      "",
      "You should start investing into activated neurons to defend yourself: for this you need oxygen and glutamate.",
      "",
      "To start building units, you first need to build a synapse.",
      "EPSP aims to destroy enemy buildings, while IPSP blocks buildings.",
      "",
      "You can first enter a number x∈{1..9} and the select epsp/ ipsp. This will automatically try to create x epsp/ ipsp.",
      "",
      "Also remember you gain less resources the more resources you have, so keep you resources low!",
      "Finally: keep track of your bound resources, the more resources are bound, the less you gain!"
    }
  };
}

#endif
