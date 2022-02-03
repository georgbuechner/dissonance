#ifndef SRC_UTILS_H_
#define SRC_UTILS_H_

#include <chrono>
#include <iterator>
#include <list>
#include <string>
#include <utility>
#include <vector>

#include "curses.h"
#include "nlohmann/json.hpp"

namespace utils {

  typedef std::pair<int, int> position_t;

  /**
   * Is up
   */
  bool IsDown(char choice);
  bool IsUp(char choice);
  bool IsLeft(char choice);
  bool IsRight(char choice);

  /**
   * Converts int to string. F.e start='a' and i=2 -> "b". Or: start='1' and i=5 -> "6"
   * @param[in] start char to start from
   * @param[in] i 
   * @returns string
   */
  std::string CharToString(char start, int i);

  /**
  * @param[in] str string to be split.
  * @param[in] delimiter 
  * @return vector
  */
  std::vector<std::string> Split(std::string str, std::string delimiter);

  /**
   * Gets elapsed milliseconds.
   * @param[in] start 
   * @param[in] end
   * @return elepased milliseconds between start and end.
   */
  double GetElapsed(std::chrono::time_point<std::chrono::steady_clock> start,
    std::chrono::time_point<std::chrono::steady_clock> end);
  double GetElapsedNano(std::chrono::time_point<std::chrono::steady_clock> start,
    std::chrono::time_point<std::chrono::steady_clock> end);

  /** 
   * Euclidean distance between two two-dimensional points.
   * @param[in] pos1
   * @param[in] pos2
   * @return euclidean distance.
   */
  double Dist(position_t pos1, position_t pos2);

  /**
   * Checks if euclidean distance between given positions satisfies given min and
   * max distance.
   * @param[in] pos1
   * @param[in] pos2
   * @param[in] min_dist
   * @param[in] max_dist
   * @return whether euclidean distance between given positions satisfies range.
   */
  bool InRange(position_t pos1, position_t pos2, double min_dist, double max_dist);

  /**
   * Gets string representation of a position in format `x|y`.
   * @param[in] pos
   * @return string representation of a position in format `x|y`.
   */
  std::string PositionToString(position_t pos);

  /**
   * Gets postion from string in format <y|x>.
   * @param[in] str_pos
   */
  position_t PositionFromString(std::string str_pos);

  /**
   * Gets postion from string in format <y|x>.
   * @param[in] str_pos
   */
  position_t PositionFromVector(std::vector<int> vec_pos);

  /**
   * Calculates true modulo of n in congruence class m.
   * @param[in] n 
   * @param[in] m (congruence class)
   * @return modulo of n in congruence class m.
   */
  unsigned int Mod(int n, int m, unsigned int min=0);

  /**
   * Converts all chars to upper case.
   * @param[in] str
   * @returns given string with all chars to upper case.
   */
  std::string ToUpper(std::string str);

  /**
   * Gets array of paths in given directory.
   * @param[in] path
   * @return array of paths in given directory.
   */
  std::vector<std::string> GetAllPathsInDirectory(std::string path);

  /**
   * Gets string representation of double value with given precision.
   * @param[in] value
   * @param[in] precision (default: 2)
   * @return string representation of double value with given precision.
   */
  std::string Dtos(double value, unsigned int precision=2);

  /**
   * Creates id in format: [type][random(0, 9) x 39].
   * @param[in] type
   * @return id in format: [type][random(0, 9) x 39].
   */
  std::string CreateId(std::string type);

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

  std::string GetMedia(std::string path);

  void StoreMedia(std::string path, std::string content);

  /**
   * Gets current date-time in YYYY-MM-DD-HH-MM-SS format.
   * @return string formated current date-time.
   */
  std::string GetFormatedDatetime();

  /**
   * @brief Parsed json from string and validates that given values exist.
   * @param keys keys which must exist.
   * @param source from which to parse json.
   * @return pair with boolean and generated json.
   */
  std::pair<bool, nlohmann::json> ValidateJson(std::vector<std::string> keys, 
      std::string source);

  void WaitABit(int milliseconds);

  /**
   * Slices vector from `begin` to `begin+len`.
   * @param[in] begin
   * @param[in] len
   * @return slices vector.
   */
  template<class T>
  std::vector<T> SliceVector(std::vector<T> in_vec, unsigned int begin, unsigned int len) {
    std::vector<T> out_vec;
    for (unsigned int i=begin; i<begin+len && i<in_vec.size(); i++)
      out_vec.push_back(in_vec[i]);
    return out_vec;
  }

  template<class K, class V>
  std::vector<V> MapToVec(std::map<K, V>& m) {
    std::vector<V> v;
    for (const auto& it : m) 
      v.push_back(it.second);
    return v;
  }
  
  template<class T>
  int Index(std::list<T> list, T elem) {
    auto it = std::find(list.begin(), list.end(), elem);
    return std::distance(list.begin(), it);
  }
}

#endif
