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

std::vector<std::vector<std::string>> utils::LoadWelcome() {
  return {
    {
      "Welcome to DISONANCE"
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
      "When disonance starts, rember you should boast oxygen and activate production of glutamate, to start defending yourself.",
      "Also keep in mind, that there are to kinds of potential: ",
      "epsp (strong in attack) and ipsp (blocks buildings); you should start with epsp."
    }
  };
}
