#include "utils.h"
#include "curses.h"
#include "nlohmann/json.hpp"
#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <math.h>
#include <vector>
#include <sstream>
#include "spdlog/spdlog.h"

#define LOGGER "logger"


bool utils::IsDown(char choice) {
  return choice == 'j' || choice == (char)KEY_DOWN;
}

bool utils::IsUp(char choice) {
  return choice == 'k' || choice == (char)KEY_UP;
}

bool utils::IsLeft(char choice) {
  return choice == 'h' || choice == (char)KEY_LEFT;
}

bool utils::IsRight(char choice) {
  return choice == 'l' || choice == (char)KEY_RIGHT;
}

double utils::GetElapsed(std::chrono::time_point<std::chrono::steady_clock> start,
    std::chrono::time_point<std::chrono::steady_clock> end) {
  return std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
}

double utils::Dist(position_t pos1, position_t pos2) {
  return std::sqrt(pow(pos2.first - pos1.first, 2) + pow(pos2.second - pos1.second, 2));
}

bool utils::InRange(position_t pos1, position_t pos2, double min_dist, double max_dist) {
  double dist = utils::Dist(pos1, pos2);
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

std::string utils::PositionToString(position_t pos) {
  return std::to_string(pos.first) + "|" + std::to_string(pos.second);
}

unsigned int utils::Mod(int n, int m) {
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

std::string utils::Dtos(double value, unsigned int precision) {
  std::stringstream stream;
  stream << std::fixed << std::setprecision(precision) << value;
  return stream.str();
}

std::string utils::CreateId(std::string type) {
  std::string id = type;
  for (int i=0; i<32; i++) {
    int ran = rand() % 9;
    id += std::to_string(ran);
  }
  return id;

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

std::string utils::GetFormatedDatetime() {
  std::time_t rawtime;
  std::tm* timeinfo;
  char buffer[80];
  std::time(&rawtime);
  timeinfo = std::localtime(&rawtime);
  std::strftime(buffer, 80, "%Y-%m-%d-%H-%M-%S",timeinfo);
  std::puts(buffer);
  return buffer;
}

std::pair<bool, nlohmann::json> utils::ValidateJson(std::vector<std::string> keys, std::string source) {
  nlohmann::json json;
  try {
    json = nlohmann::json::parse(source); 
  }
  catch (std::exception& e) {
    spdlog::get(LOGGER)->warn("ValidateJson: Failed parsing json: {}", e.what());
    return std::make_pair(false, nlohmann::json());
  }
  for (auto key : keys) {
    std::vector<std::string> depths = Split(key, "/");
    if (depths.size() > 0) {
      if (json.count(depths[0]) == 0) {
        spdlog::get(LOGGER)->info("ValidateJson: Missing key: {}", key);
        return std::make_pair(false, json);
      }
    }
    if (depths.size() > 1) {
      if (json[depths[0]].count(depths[1]) == 0) {
        spdlog::get(LOGGER)->info("ValidateJson: Missing key: {}", key);
        return std::make_pair(false, json);
      }
    }
  }
  return std::make_pair(true, json);
}
