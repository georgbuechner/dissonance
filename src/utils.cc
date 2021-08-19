#include "utils.h"
#include <math.h>
#include <vector>

double utils::get_elapsed(std::chrono::time_point<std::chrono::steady_clock> start,
    std::chrono::time_point<std::chrono::steady_clock> end) {
  return std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
}

int utils::dist(Position pos1, Position pos2) {
  return std::sqrt(pow(pos2.first - pos1.first, 2) + pow(pos2.second - pos1.second, 2));
}

std::string utils::PositionToString(Position pos) {
  return std::to_string(pos.first) + "|" + std::to_string(pos.second);
}

int utils::getrandom_int(int min, int max) {
  int ran = min + (rand() % (max - min + 1)); 
  return ran;
}

std::string utils::create_id() {
  std::string id = "";
  for (int i=0; i<10; i++) {
    int ran = rand() % 9;
    id += std::to_string(ran);
  }
  return id;

}

utils::Paragraphs utils::LoadWelcome() {
  return {
    {
      "Welcome to DISSONANCE"
    },
    {
      "Each player starts with a nucleus.",
      "One nucleus has control over a few cells surrounding it.",
      "By gathering different resources (iron, oxygen, potassium, ...) you can create Synapses to generate potential, advancing towards the enemies nucleus.",
      "When a certain amount of potential has reached the enemies nucleus, your enemy is destroyed.",
      "By activating cells you control, these cells can neutralize incoming potential."
    },
    {
      "You randomly gain iron every few seconds (the more the game advances the less iron you gain).",
      "Iron can be used to activate the process of gathering new resources or to boost your oxygen production.",
      "Depending on your current oxygen level, you gain more or less resources.",
      "Oxygen is also needed to create Synapses or to activate sells for your defences. But be careful:",
      "the more oxygen you spend on building advanced neurons (Synapses/ activated neurons) the less resources you gain per seconds!"
    },
    {
      "Once you started gaining dopamine and serotonin, you can develop advanced technologies, allowing you f.e. to target specific enemy neurons and hence destroy enemy synapses or activated neurons.",
      "Other technologies or advanced use of potentials are waiting for you...\n\n"
    },
    {
      "When dissonance starts, remember you should boast oxygen and activate production of glutamate, to start defending yourself.",
      "Also keep in mind, that there are two kinds of potential: ",
      "EPSP (strong in attack) and IPSP (blocks buildings); you should start with EPSP."
    }
  };
}

utils::Paragraphs utils::LoadHelp() {
  return {
    {
      "##### HELP #####",
      "",
      "--- COSTS ----", 
      "ACTIVATE NEURON: oxygen=8.9, glutamate=19.1",
      "SYNAPSE: oxygen=13.4, potassium=6.6",
      "EPSP: potassium=4.4",
      "IPSP: potassium=3.4, chloride=6.8",
    },
    {
      "##### HELP #####",
      "",
      "--- TIPS ----", 
      "Iron is used to boast oxygen production (1 iron per boast) or to start gaining new resources (2 iron per new resource).",
      "Iron is gained in relation to you oxygen-level: more oxygen = less iron per second",
      "",
      "You should start investing into activate neurons to defend yourself: for this you need oxygen and glutamate.",
      "",
      "To start building units, you first need to build a synapse.",
      "EPSP aims to destroy enemy buildings, while IPSP blocks buildings.",
      "",
      "Also remember you gain resources from FREE oxygen. The more bound oxygen you have, then less resources you gain!"
    }
  };
}
