#ifndef SRC_DATA_STRUCTS_H_
#define SRC_DATA_STRUCTS_H_

#include <map>
#include <string>
#include <vector>

struct Options {
  std::vector<size_t> options_;
  std::map<size_t, size_t> mapping_option_to_func_;
  std::map<size_t, std::string> mapping_option_to_desc_;

  void AddOption(size_t index, size_t function_mapper, std::string desc) {
    options_.push_back(index);
    mapping_option_to_func_[index] = function_mapper;
    mapping_option_to_desc_[index] = desc;
  }
};

#endif
