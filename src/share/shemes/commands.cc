#include "share/shemes/commands.h"
#include "share/shemes/data.h"
#include <memory>
#include <msgpack.hpp>

Command::Command(std::string command) {
  command_ = command;
  data_ = std::make_shared<Data>();
}

Command::Command(std::string command, std::shared_ptr<Data> data) {
  command_ = command;
  data_ = data;
}

Command::Command(std::string command, std::string username, std::shared_ptr<Data> data) {
  command_ = command;
  username_ = username;
  data_ = data;
}

Command::Command(const char* payload, size_t len) {
  size_t offset = 0;
  msgpack::object_handle result;

  // get command
  unpack(result, payload, len, offset);
  command_ = result->as<std::string>();
  // get username
  unpack(result, payload, len, offset);
  username_ = result->as<std::string>();

  // get data dependant on command.
  if (command_ == "kill")
    data_ = std::make_shared<Msg>(payload, len, offset);
  else if (command_ == "send_audio_info")
    data_ = std::make_shared<SendAudioInfo>(payload, len, offset);
  else if (command_ == "print_msg")
    data_ = std::make_shared<Msg>(payload, len, offset);
  else if (command_ == "set_msg")
    data_ = std::make_shared<Msg>(payload, len, offset);
  else if (command_ == "select_mode")
    data_ = std::make_shared<Paragraph>(payload, len, offset);
  else if (command_ == "set_unit")
    data_ = std::make_shared<FieldPosition>(payload, len, offset);
  else if (command_ == "set_units")
    data_ = std::make_shared<Units>(payload, len, offset);
  else if (command_ == "update_game")
    data_ = std::make_shared<Update>(payload, len, offset);
  else if (command_ == "setup_new_game")
    data_ = std::make_shared<InitNewGame>(payload, len, offset);
  else if (command_ == "init_game")
    data_ = std::make_shared<Init>(payload, len, offset);
  else if (command_ == "update_lobby")
    data_ = std::make_shared<Lobby>(payload, len, offset);
  else if (command_ == "game_end")
    data_ = std::make_shared<GameEnd>(payload, len, offset);
  else if (command_ == "build_potential" || command_ == "check_build_potential")
    data_ = std::make_shared<BuildPotential>(payload, len, offset);
  else if (command_ == "build_neuron" || command_ == "check_build_neuron")
    data_ = std::make_shared<BuildNeuron>(payload, len, offset);
  else if (command_ == "select_synapse")
    data_ = std::make_shared<SelectSynapse>(payload, len, offset);
  else if (command_ == "set_wps" || command_ == "set_way_point")
    data_ = std::make_shared<SetWayPoints>(payload, len, offset);
  else if (command_ == "set_target")
    data_ = std::make_shared<SetTarget>(payload, len, offset);
  else if (command_ == "toggle_swarm_attack")
    data_ = std::make_shared<ToggleSwarmAttack>(payload, len, offset);
  else if (command_ == "get_positions")
    data_ = std::make_shared<GetPositions>(payload, len, offset);
  else if (command_ == "audio_map")
    data_ = std::make_shared<CheckSendAudio>(payload, len, offset);
  else if (command_ == "send_audio_data")
    data_ = std::make_shared<AudioTransferData>(payload, len, offset);
  else if (command_ == "initialize_game")
    data_ = std::make_shared<InitializeGame>(payload, len, offset);
  else if (command_ == "add_iron")
    data_ = std::make_shared<DistributeIron>(payload, len, offset);
  else if (command_ == "remove_iron")
    data_ = std::make_shared<DistributeIron>(payload, len, offset);
  else if (command_ == "add_technology")
    data_ = std::make_shared<AddTechnology>(payload, len, offset);
  else
    data_ = std::make_shared<Data>();
}

// getter 
std::string Command::command() { return command_; }
std::string Command::username() { return username_; }
std::shared_ptr<Data> Command::data() { return data_; }

// setter
void Command::set_username(std::string username) { username_ = username; }

// methods 
std::string Command::bytes() {
  std::stringstream buffer;
  msgpack::pack(buffer, command_);
  msgpack::pack(buffer, username_);
  data_->binary(buffer);
  return buffer.str();
}
