cmake_minimum_required(VERSION 3.10)

# set the project name and version
project(strategy VERSION 1.0)
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

# specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
SET(CMAKE_CXX_FLAGS "-Wall -Werror -pthread")
add_compile_options(-fdiagnostics-color=always)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)

# Add source files to create textadventure
set(SRC_FILES
    src/main.cc
    src/field.cc
    src/game.cc
)

add_executable(game ${SRC_FILES})

target_link_libraries(game PUBLIC ${CONAN_LIBS})

target_include_directories(game PUBLIC
  "src"
)
