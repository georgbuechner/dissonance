#include "utils.h"
#include "nlohmann/json.hpp"
#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <math.h>
#include <vector>
#include "spdlog/spdlog.h"

#define LOGGER "logger"


double utils::get_elapsed(std::chrono::time_point<std::chrono::steady_clock> start,
    std::chrono::time_point<std::chrono::steady_clock> end) {
  return std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
}

double utils::dist(Position pos1, Position pos2) {
  return std::sqrt(pow(pos2.first - pos1.first, 2) + pow(pos2.second - pos1.second, 2));
}

bool utils::InRange(Position pos1, Position pos2, double min_dist, double max_dist) {
  double dist = utils::dist(pos1, pos2);
  return dist >= min_dist && dist <= max_dist;
}

std::vector<std::string> utils::Split(std::string str, std::string delimiter) {
  std::vector<std::string> v_strs;
  size_t pos=0;
  while ((pos = str.find(delimiter)) != std::string::npos) {
    if(pos!=0)
        v_strs.push_back(str.substr(0, pos));
    str.erase(0, pos + delimiter.length());
  }
  v_strs.push_back(str);

  return v_strs;
}


std::string utils::PositionToString(Position pos) {
  return std::to_string(pos.first) + "|" + std::to_string(pos.second);
}

int utils::getrandom_int(int min, int max) {
  int ran = min + (rand() % (max - min + 1)); 
  return ran;
}

unsigned int utils::mod(int n, int m) {
  return ((n%m)+m)%m;
}

std::string utils::ToUpper(std::string str) {
  std::string upper;
  for (const auto& c : str) {
    upper += std::toupper(c); 
  }
  return upper;
}

std::vector<std::string> utils::GetAllPathsInDirectory(std::string path) {
  std::vector<std::string> paths;
  for (const auto& it : std::filesystem::directory_iterator(path))
    paths.push_back(it.path().string());
  return paths;
}

std::string utils::create_id(std::string type) {
  std::string id = type;
  for (int i=0; i<32; i++) {
    int ran = rand() % 9;
    id += std::to_string(ran);
  }
  return id;

}

Options utils::CreateOptionsFromStrings(std::vector<std::string> option_strings) {
  std::vector<size_t> options;
  std::map<size_t, std::string> mappings;
  size_t counter = 0;
  for (const auto& it : option_strings) {
    options.push_back(++counter);
    mappings[counter] = it;
  }
  return Options({options, {}, mappings});
}

nlohmann::json utils::LoadJsonFromDisc(std::string path) {
  nlohmann::json json;
  std::ifstream read(path.c_str());
  if (!read) {
    spdlog::get(LOGGER)->error("Audio::Safe: Could not open file at {}", path);
    return json;
  }
  try {
    read >> json;
  } 
  catch (std::exception& e) {
    spdlog::get(LOGGER)->error("Audio::Safe: Could not read json.");
    read.close();
    return json;
  }
  // Success 
  read.close();
  return json;
}

void utils::WriteJsonFromDisc(std::string path, nlohmann::json& json) {
  std::ofstream write(path.c_str());
  if (!write)
    spdlog::get(LOGGER)->error("Audio::Safe: Could not safe at {}", path);
  else {
    spdlog::get(LOGGER)->info("Audio::Safe: safeing at {}", path);
    write << json;
  }
  write.close();
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
      "--- COSTS (Potential/ Neurons) ----", 
      "ACTIVATE NEURON: oxygen=8.9, glutamate=19.1",
      "SYNAPSE: oxygen=13.4, potassium=6.6",
      "EPSP: potassium=4.4",
      "IPSP: potassium=3.4, chloride=6.8",
    },
    {
      "##### HELP #####",
      "",
      "--- COSTS (Potential/ Neurons) ----", 
      "WAY (select way/ way-points for neurosn): dopamine=7.7",
      "SWARM (launch swarm attacks +3): dopamine=9.9",
      "TARGET (choose target: ipsp/ epsp): dopamine=6.5",
      "TOTAL OXYGEN (max allowed oxygen bound+free): dopamine=7.5, serotonin=8.9",
      "TOTAL RESOURCE (max allowed resources: each): dopamine=8.5, serotonin=7.9",
      "CURVE (resource curve slowdown): dopamine=11.0, serotonin=11.2",
      "POTENIAL (increases potential of ipsp/ epsp): dopamine=5.0, serotonin=11.2",
      "SPEED (increases speed of ipsp/ epsp): dopamine=3.0, serotonin=11.2",
      "DURATION (increases duration at target of ipsp): dopamine=2.5, serotonin=11.2)",
    },
    {
      "##### HELP #####",
      "",
      "--- TIPS ----", 
      "Iron is used to boast oxygen production (1 iron per boast) or to start gaining new resources (2 iron per new resource).",
      "Iron is gained in relation to you oxygen-level: you only gain iron if oxygen is below 10 and you may never have more than 3 iron at a time!",
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
