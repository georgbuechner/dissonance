#include "utils.h"
#include "curses.h"
#include "nlohmann/json.hpp"
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <ctime>
#include <exception>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <math.h>
#include <string>
#include <vector>
#include <sstream>
#include "share/objects/units.h"
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

double utils::GetElapsedNano(std::chrono::time_point<std::chrono::steady_clock> start,
    std::chrono::time_point<std::chrono::steady_clock> end) {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count();
}

double utils::Dist(position_t pos1, position_t pos2) {
  return std::sqrt(pow(pos2.first - pos1.first, 2) + pow(pos2.second - pos1.second, 2));
}

bool utils::InRange(position_t pos1, position_t pos2, double min_dist, double max_dist) {
  double dist = utils::Dist(pos1, pos2);
  return dist >= min_dist && dist <= max_dist;
}


std::string utils::CharToString(char start, int i) {
  char c_str = start+i;
  std::string str = "";
  str += c_str;
  return str;
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

position_t utils::PositionFromString(std::string str_pos) {
  return {std::stoi(Split(str_pos, "|")[0]), std::stoi(Split(str_pos, "|")[1])};
}

position_t utils::PositionFromVector(std::vector<int> vec_pos) {
  return {vec_pos[0], vec_pos[1]};
}

int utils::Mod(int n, int m, int min) {
  int val = ((n%m)+m)%m;
  if (val < min)
    val = min;
  return val;
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

std::string utils::Dtos(double value, int precision) {
  std::stringstream stream;
  stream << std::fixed << std::setprecision(precision) << value;
  return stream.str();
}

std::string utils::CreateId(std::string type) {
  std::string id = type;
  for (int i=0; i<16; i++) {
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

void utils::WriteJsonToDisc(std::string path, nlohmann::json& json) {
  std::ofstream write(path.c_str());
  if (!write)
    spdlog::get(LOGGER)->error("utils::WriteJsonToDisc: Could not safe at {}", path);
  else {
    spdlog::get(LOGGER)->info("utils::WriteJsonToDisc: safeing at {}", path);
    write << json;
  }
  write.close();
}

std::string utils::GetMedia(std::string path) {
  std::ifstream f(path, std::ios::in|std::ios::binary|std::ios::ate);
  if (!f.is_open()) {
    spdlog::get(LOGGER)->warn("file could not be found! {}", path);
    return "";
  }
  else {
    FILE* file_stream = fopen(path.c_str(), "rb");
    std::vector<char> buffer;
    fseek(file_stream, 0, SEEK_END);
    long length = ftell(file_stream);
    rewind(file_stream);
    buffer.resize(length);
    length = fread(&buffer[0], 1, length, file_stream);
    std::string s(buffer.begin(), buffer.end());
    return s;
  }
}

void utils::StoreMedia(std::string path, std::string content) {
  try {
    std::fstream mediaout(path, std::ios::out | std::ios::binary);
    mediaout.write(content.c_str(), content.size());
    mediaout.close();
  }
  catch (std::exception& e) {
    std::cout << "Writing media-file failed: " << e.what() << std::endl;
    return;
  }
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

void utils::WaitABit(int milliseconds) {
  // Wait a bit.
  auto cur_time = std::chrono::steady_clock::now();
  while (utils::GetElapsed(cur_time, std::chrono::steady_clock::now()) < milliseconds) {}
}

struct Line {
  Line(std::pair<double, double> p1, std::pair<double, double> p2) {
    _a = p2.second-p1.second;
    _b = p1.first-p2.first;
    _c = (_a*p1.first + _b*p1.second) * (-1);
  }
  double _a;
  double _b;
  double _c;
};

double ShortestDistance(double x, double y, Line line) {
  return std::abs((line._a*x + line._b*y + line._c) / std::sqrt(line._a*line._a + line._b*line._b));
}

double utils::GetPercentDiff(double a, double b) {
  return std::abs((a-b)/a) * 100;
}

std::vector<std::pair<int, double>> utils::DouglasPeucker(const std::vector<std::pair<int, double>>& vec, double e) {
  // Find the point with the maximum distance
  double dmax =  0;
  int index = 0;
  int end = vec.size()-1;
  for (int i=1; i<end-1; i++) {
    double d = ShortestDistance(vec[i].first, vec[i].second, Line(vec[0], vec[end]));
    if (d > dmax) {
      index = i;
      dmax = d;
    }
  }

  std::vector<std::pair<int, double>> result;
  if (dmax > e) {
    // Recursive call
    std::vector<std::pair<int, double>> part_1(vec.begin(), vec.begin()+index);
    auto results_1 = DouglasPeucker(part_1, e);
    std::vector<std::pair<int, double>> part_2(vec.begin()+index, vec.end());
    auto results_2 = DouglasPeucker(part_2, e);
    // Build result list
    result.insert(result.end(), results_1.begin(), results_1.end());
    result.insert(result.end(), results_2.begin(), results_2.end());
  }
  else {
    result = {vec[0], vec[end]};
  }
  return result;
}

double utils::GetEpsilon(int num_pitches, int max_elems) {
  double e = 1*std::pow(10, std::sqrt(num_pitches/max_elems)-1);
  return (e > 10000) ? 10000 : e;
}
