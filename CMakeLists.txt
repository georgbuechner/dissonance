cmake_minimum_required(VERSION 3.10)
# set the project name and version
project(dissonance VERSION 1.0)
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

# specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
if(APPLE)
  SET(CMAKE_CXX_FLAGS "-pthread -ldl -lm")
else()
  SET(CMAKE_CXX_FLAGS "-Wall -Werror -pthread -ldl -lm -lncursesw")
endif(APPLE)
add_compile_options(-fdiagnostics-color=always)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)

# Add source files needed for tests and game
set(SRC_FILES
  src/client/game/client_game.cc
  src/client/game/print/drawrer.cc
  src/client/websocket/client.cc

  src/share/audio/audio.cc
  src/share/audio/miniaudio.cc
  src/share/objects/units.cc
  src/share/objects/resource.cc
  src/share/tools/utils/utils.cc
  src/share/tools/random/random.cc
  src/share/shemes/commands.cc
  src/share/shemes/data.cc

  src/server/game/server_game.cc
  src/server/game/field/field.cc
  src/server/game/player/audio_ki.cc
  src/server/game/player/monto_carlo_ai.cc
  src/server/game/player/player.cc
  src/server/websocket/websocket_server.cc
)

# Add soruce files needed for game only
set(SRC_FILES_GAME 
  src/main.cc 
  ${SRC_FILES}
)

# Add source files for testing only
set(SRC_FILES_TEST
  test/main.cc
  test/test_audio.cc
  test/test_field.cc
  test/test_player.cc
  test/test_utils.cc
  test/test_units.cc
  test/test_proto.cc
  test/testing_utils.cc
  ${SRC_FILES}
)

include_directories(/usr/local/lib/)
link_directories(/usr/local/lib/)

add_executable(dissonance ${SRC_FILES_GAME})
add_executable(tests ${SRC_FILES_TEST})

target_link_libraries(dissonance PUBLIC aubio ${CONAN_LIBS})
target_link_libraries(tests PUBLIC aubio ${CONAN_LIBS})

target_include_directories(dissonance PUBLIC "src")
target_include_directories(tests PUBLIC "src" "test")
