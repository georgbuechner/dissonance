cmake_minimum_required(VERSION 3.10)

# set the project name and version
project(strategy VERSION 1.0)
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

# specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
SET(CMAKE_CXX_FLAGS "-Wall -Werror -pthread -lncursesw")
add_compile_options(-fdiagnostics-color=always)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)

# Add source files needed for tests and game
set(SRC_FILES
  src/field.cc
  src/game.cc
  src/player.cc
  src/utils.cc
  src/units.cc
)

# Add soruce files needed for game only
set(SRC_FILES_GAME 
  src/main.cc 
  ${SRC_FILES}
)

# Add source files for testing only
set(SRC_FILES_TEST
  test/main.cc
  test/test_field.cc
  test/test_utils.cc
  test/test_units.cc
  ${SRC_FILES}
)

add_executable(game ${SRC_FILES_GAME})
add_executable(tests ${SRC_FILES_TEST})

target_link_libraries(game PUBLIC ${CONAN_LIBS})
target_link_libraries(tests PUBLIC ${CONAN_LIBS})

target_include_directories(game PUBLIC "src")
target_include_directories(tests PUBLIC "src" "test")
