#ifndef SRC_UTILS_H_
#define SRC_UTILS_H_

#include "nlohmann/json.hpp"
#include <chrono>
#include <string>
#include <utility>
#include <vector>

namespace utils {

  typedef std::pair<int, int> Position;
  typedef std::vector<std::vector<std::string>> Paragraphs;

  /**
  * @param[in] str string to be splitet
  * @param[in] delimitter 
  * @return vector
  */
  std::vector<std::string> Split(std::string str, std::string delimiter);

  double get_elapsed(std::chrono::time_point<std::chrono::steady_clock> start,
    std::chrono::time_point<std::chrono::steady_clock> end);

  double dist(Position pos1, Position pos2);
  bool InRange(Position pos1, Position pos2, double min_dist, double max_dist);

  std::string PositionToString(Position pos);

  int getrandom_int(int min, int max);

  unsigned int mod(int n, int m);

  std::string ToUpper(std::string str);
  std::vector<std::string> GetAllPathsInDirectory(std::string path);

  std::string dtos(double value);

  template<class T>
  std::vector<T> SliceVector(std::vector<T> in_vec, unsigned int begin, unsigned int len) {
    std::vector<T> out_vec;
    for (unsigned int i=begin; i<begin+len && i<in_vec.size(); i++)
      out_vec.push_back(in_vec[i]);
    return out_vec;
  }
  
  Paragraphs LoadWelcome();
  Paragraphs LoadHelp();

  std::string create_id(std::string type);

  /** 
   * @brief Loads json from disc
   * @param[in] path path to json.
   * @return boolean indicating success.
   */
  nlohmann::json LoadJsonFromDisc(std::string path);

  /** 
   * @brief Loads json from disc
   * @param[in] path path to json.
   * @param[out] reference to json.
   * @return boolean indicating success.
   */
  void WriteJsonFromDisc(std::string path, nlohmann::json& json);
}

#endif
