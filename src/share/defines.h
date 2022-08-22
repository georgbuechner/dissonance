#ifndef SRC_SHARE_DEFINES_H_
#define SRC_SHARE_DEFINES_H_

#include <iostream>
#include <map>
#include <string>
#include <vector>

// Colors 

#define COLOR_PAIR_BLUE 1
#define COLOR_PAIR_GREEN 2
#define COLOR_PAIR_CYAN 3
#define COLOR_PAIR_RED 4
#define COLOR_PAIR_MAGENTA 5
#define COLOR_PAIR_BROWN 6

#define COLOR_AVAILIBLE COLOR_PAIR_BLUE
#define COLOR_ERROR COLOR_RED 
#define COLOR_DEFAULT 7 
#define COLOR_MSG COLOR_PAIR_CYAN 
#define COLOR_SUCCESS COLOR_PAIR_GREEN
#define COLOR_MARKED COLOR_PAIR_MAGENTA

#define COLOR_KI COLOR_RED
#define COLOR_PLAYER COLOR_BLUE
#define COLOR_RESOURCES COLOR_PAIR_CYAN

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
#define CONTEXT_POST_GAME 5
#define CONTEXT_LOBBY 6
#define CONTEXT_TUTORIAL 7
#define CONTEXT_TEXT 8

#define VP_FIELD CONTEXT_FIELD
#define VP_RESOURCE CONTEXT_RESOURCES
#define VP_TECH CONTEXT_TECHNOLOGIES
#define VP_POST_GAME CONTEXT_POST_GAME
#define VP_LOBBY CONTEXT_LOBBY
#define VP_TEXT CONTEXT_TEXT

// logger
#define LOGGER "logger"

// typedefs

/// Map int (id) to pair (text, color)
typedef std::map<size_t, std::pair<std::string, int>> choice_mapping_t; 

/// Heigh, Width
typedef std::pair<short, short> position_t;

/// i of m
typedef std::pair<size_t, size_t> tech_of_t;

/// topline (vector of string if color paired to each string)
typedef std::vector<std::pair<std::string, int>> t_topline;

const position_t DEFAULT_POS = {-1, -1};

#endif
