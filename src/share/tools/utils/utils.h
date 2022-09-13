#ifndef SRC_UTILS_H_
#define SRC_UTILS_H_

#include <chrono>
#include <iostream>
#include <cmath>
#include <cstddef>
#include <iterator>
#include <list>
#include <string>
#include <utility>
#include <valarray>
#include <vector>

#include "nlohmann/json.hpp"

namespace utils {

  typedef std::pair<int16_t, int16_t> position_t;

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
  int Mod(int n, int m, int min=0);

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
  std::string Dtos(double value, int precision=2);

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
   * Writes json to disc
   * @param[in] path path to json.
   * @param[out] reference to json.
   * @return boolean indicating success.
   */
  void WriteJsonToDisc(std::string path, nlohmann::json& json);

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

  void SplitLargeData(std::map<int, std::string>& contents, std::string content, size_t threshold);

  /** 
   * Decimates curve, while perserving structure.
   * @param[in] vec vector of data-points (index: value)
   * @param[in] e (epsilon)
   * @return reduced curve
   */
  std::vector<std::pair<int, double>> DouglasPeucker(std::vector<std::pair<int, double>> vec, double e);

  /**
   * Finds aproxiamte epsilon based on difference of number of data points and max
   * allowed number of data points.
   * @param[in] num_pitches
   * @param[in] max_elems
   * @return aproxiamte epsilon
   */
  double GetEpsilon(int num_pitches, int max_elems);

  double GetPercent(double a, double b);

  template<class T>
  std::vector<std::pair<int, T>> DecimateCurve(std::vector<T> vec, int max_elems) {
    double epsilon = GetEpsilon(vec.size(), max_elems);
    // Convert simple vector to vector-with indexes
    std::vector<std::pair<int, double>> vec_points;
    for (size_t i=0; i<vec.size(); i++)
      vec_points.push_back({i, vec[i]});
    auto decimated_vec_points = DouglasPeucker(vec_points, epsilon);
    double max_epsilon = std::sqrt(vec.size());
    double min_epsilon = 0;
    for (int i=0; i<10; i++) {
      if (GetPercent(decimated_vec_points.size(), max_elems) < 7)
        break;
      if (decimated_vec_points.size() > (size_t)max_elems) {
        min_epsilon = epsilon;
        epsilon = (epsilon+max_epsilon)/2;
      }
      else {
        max_epsilon = epsilon;
        epsilon = (epsilon+min_epsilon)/2;
      }
      decimated_vec_points = DouglasPeucker(vec_points, epsilon);
    }
    std::vector<std::pair<int, T>> vec_points_t;
    for (const auto& it : decimated_vec_points) {
      vec_points_t.push_back({it.first, it.second});
    }
    return vec_points_t;
  }

  template<class T>
  std::vector<T> DecimateCurveReconverted(std::vector<T> vec, int max_elems) {
    auto decimated_vec_points = DecimateCurve(vec, max_elems);
    // re-convert to simple vector
    std::vector<T> decimated_curve;
    for (const auto& x : decimated_vec_points)
      decimated_curve.push_back(x.second);
    return decimated_curve;
  }

  /**
   * Slices vector from `begin` to `begin+len`.
   * @param[in] begin
   * @param[in] len
   * @return slices vector.
   */
  template<class T>
  std::vector<T> SliceVector(std::vector<T> in_vec, int begin, int len) {
    std::vector<T> out_vec;
    for (int i=begin; i<begin+len && (size_t)i<in_vec.size(); i++)
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
