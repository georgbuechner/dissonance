#ifndef SRC_SHARE_DEFINES_H_
#define SRC_SHARE_DEFINES_H_

#include <iostream>
#include <map>
#include <string>

// Colors 
#define COLOR_AVAILIBLE 1
#define COLOR_ERROR 2 
#define COLOR_DEFAULT 3
#define COLOR_MSG 4
#define COLOR_SUCCESS 5
#define COLOR_MARKED 6
#define COLOR_PROGRESS 7

#define COLOR_KI 2
#define COLOR_PLAYER 1
#define COLOR_RESOURCES 4 

#define COLOR_P2 10
#define COLOR_P3 11
#define COLOR_P4 12
#define COLOR_P5 13
#define COLOR_P6 14

// Contexts 
#define CONTEXT_FIELD 0
#define CONTEXT_RESOURCES 1
#define CONTEXT_TECHNOLOGIES 2
#define CONTEXT_PICK 3 
#define CONTEXT_SYNAPSE 4

#define VP_FIELD CONTEXT_FIELD
#define VP_RESOURCE CONTEXT_RESOURCES
#define VP_TECH CONTEXT_TECHNOLOGIES

// logger
#define LOGGER "logger"

// typedefs

/// Map int (id) to pair (text, color)
typedef std::map<size_t, std::pair<std::string, int>> choice_mapping_t; 

/// Heigh, Width
typedef std::pair<int, int> position_t;

/// i of m
typedef std::pair<size_t, size_t> tech_of_t;

#endif
