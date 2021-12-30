#include "client/game/client_game.h"
#include "client/game/print/drawrer.h"
#include "client/websocket/client.h"
#include "nlohmann/json_fwd.hpp"
#include "share/constants/codes.h"
#include "share/constants/texts.h"
#include "share/defines.h"
#include "share/objects/units.h"
#include "share/objects/transfer.h"
#include "share/tools/eventmanager.h"
#include "share/tools/utils/utils.h"

#include "spdlog/spdlog.h"

#include <cctype>
#include <filesystem>
#include <string>
#include <unistd.h>
#include <utility>

#define CONTEXT_RESOURCES_MSG "Distribute (+)/ remove (-) iron to handler resource-gain"
#define CONTEXT_TECHNOLOGIES_MSG "Research technology by pressing [enter]"
#define CONTEXT_SYNAPSE_MSG "Select instructions for this synapse"

std::vector<std::pair<std::string, int>> std_topline = {{" [i]psp ", COLOR_DEFAULT}, 
  {" [e]psp  ", COLOR_DEFAULT}, {" [A]ctivated Neuron  ", COLOR_DEFAULT}, 
  {" [S]ynape ", COLOR_DEFAULT}, {" [s]elect-synapse ", COLOR_DEFAULT}, 
  {" [t]oggle-navigation ", COLOR_DEFAULT}, {" [h]elp ", COLOR_DEFAULT}, {" [q]uit ", COLOR_DEFAULT}};

std::vector<std::pair<std::string, int>> field_topline = {{" [h, j, k, l] to navigate field "
  " [t]oggle-navigation ", COLOR_DEFAULT}, {" [h]elp ", COLOR_DEFAULT}, {" [q]uit ", COLOR_DEFAULT}};

std::vector<std::pair<std::string, int>> synapse_topline = {{" [s]et way-point ", COLOR_DEFAULT}, 
  {" [a]dd way-point ", COLOR_DEFAULT}, {" [i]psp-target ", COLOR_DEFAULT}, 
  { " [e]psp-target ", COLOR_DEFAULT}, { " toggle s[w]arm-attack ", COLOR_DEFAULT},
  {" [t]oggle-navigation ", COLOR_DEFAULT}, {" [h]elp ", COLOR_DEFAULT}, {" [q]uit ", COLOR_DEFAULT}
};

ClientGame::ClientGame(bool relative_size, std::string base_path, std::string username, bool mp) 
    : username_(username), muliplayer_availible_(mp), base_path_(base_path), render_pause_(false), drawrer_() {
  status_ = WAITING;
  // Initialize curses
  setlocale(LC_ALL, "");
  initscr();
  cbreak();
  noecho();
  curs_set(0);
  keypad(stdscr, true);
  clear();
  
  // Initialize colors.
  use_default_colors();
  start_color();
  init_pair(COLOR_AVAILIBLE, COLOR_BLUE, -1);
  init_pair(COLOR_ERROR, COLOR_RED, -1);
  init_pair(COLOR_DEFAULT, -1, -1);
  init_pair(COLOR_MSG, COLOR_CYAN, -1);
  init_pair(COLOR_SUCCESS, COLOR_GREEN, -1);
  init_pair(COLOR_MARKED, COLOR_MAGENTA, -1);

  // Setup map-size
  drawrer_.SetUpBorders(LINES, COLS);

  // Set-up audio-paths.
  std::vector<std::string> paths = utils::LoadJsonFromDisc(base_path_ + "/settings/music_paths.json");
  for (const auto& it : paths) {
    if (it.find("$(HOME)") != std::string::npos)
      audio_paths_.push_back(getenv("HOME") + it.substr(it.find("/")));
    else if (it.find("$(DISSONANCE)") != std::string::npos)
      audio_paths_.push_back(base_path_ + it.substr(it.find("/")));
    else
      audio_paths_.push_back(it);
  }

  // Initialize contexts
  current_context_ = CONTEXT_RESOURCES;
  // Basic handlers shared by standard-contexts
  std::map<char, void(ClientGame::*)(int)> std_handlers = { {'j', &ClientGame::h_MoveSelectionUp}, 
    {'k', &ClientGame::h_MoveSelectionDown}, {'t', &ClientGame::h_ChangeViewPoint}, {'q', &ClientGame::h_Quit},
    {'A', &ClientGame::h_BuildActivatedNeuron}, {'S', &ClientGame::h_BuildSynapse}, {'s', &ClientGame::h_SendSelectSynapse},
    {'e', &ClientGame::h_BuildEpsp}, {'i', &ClientGame::h_BuildIpsp}
  };
  // field context 
  contexts_[CONTEXT_FIELD] = Context("", std_handlers, {{'h', &ClientGame::h_MoveSelectionLeft},
      {'l', &ClientGame::h_MoveSelectionRight}, {'\n', &ClientGame::h_BuildNeuron}}, field_topline);
  // Resource context:
  contexts_[CONTEXT_RESOURCES] = Context(CONTEXT_RESOURCES_MSG, std_handlers, {{'+', &ClientGame::h_AddIron}, 
      {'-', &ClientGame::h_RemoveIron}}, std_topline);
  // Technology context:
  contexts_[CONTEXT_TECHNOLOGIES] = Context(CONTEXT_TECHNOLOGIES_MSG, std_handlers, {{'\n', &ClientGame::h_AddTech}},
      std_topline);
  // Synapse context:
  std::map<char, void(ClientGame::*)(int)> synapse_handlers = {{'s', &ClientGame::h_SetWP},
      {'a', &ClientGame::h_AddWP}, {'i', &ClientGame::h_IpspTarget}, 
      {'e', &ClientGame::h_EpspTarget}, {'w', &ClientGame::h_SwarmAttack},
      {'t', &ClientGame::h_ChangeViewPoint}, {'q', &ClientGame::h_Quit}};
  contexts_[CONTEXT_SYNAPSE] = Context(CONTEXT_SYNAPSE_MSG, synapse_handlers, synapse_topline);

  // Initialize eventmanager.
  eventmanager_.AddHandler("select_mode", &ClientGame::m_SelectMode);
  eventmanager_.AddHandler("select_audio", &ClientGame::m_SelectAudio);
  eventmanager_.AddHandler("print_msg", &ClientGame::m_PrintMsg);
  eventmanager_.AddHandler("init_game", &ClientGame::m_InitGame);
  eventmanager_.AddHandler("update_game", &ClientGame::m_UpdateGame);
  eventmanager_.AddHandler("set_msg", &ClientGame::m_SetMsg);
  eventmanager_.AddHandler("game_end", &ClientGame::m_GameEnd);
  eventmanager_.AddHandler("select_position", &ClientGame::m_SelectPosition);
  eventmanager_.AddHandler("select_position_synapse", &ClientGame::m_SelectPositionSynapse);
  eventmanager_.AddHandler("set_unit", &ClientGame::m_SetUnit);
  eventmanager_.AddHandler("set_units", &ClientGame::m_SetUnits);
  eventmanager_.AddHandler("change_context", &ClientGame::m_ChangeContext);
}

nlohmann::json ClientGame::HandleAction(nlohmann::json msg) {
  std::string command = msg["command"];
  spdlog::get(LOGGER)->debug("ClientGame::HandleAction: {}", command);

  // Get event from event-manager
  if (eventmanager_.handlers().count(command))
    (this->*eventmanager_.handlers().at(command))(msg);
  else 
    msg = nlohmann::json(); // if no matching event set return-message to empty.
  
  spdlog::get(LOGGER)->debug("ClientGame::HandleAction: response {}", msg.dump());
  return msg;
}


void ClientGame::GetAction() {
  spdlog::get(LOGGER)->info("ClientGame::GetAction stated");

  while(true) {
    // Get Input
    if (status_ == WAITING) continue; // Skip as long as not active.
    if (status_ == CLOSING) break; // Leave thread.
    char choice = getch();
    history_.push_back(choice);
    spdlog::get(LOGGER)->info("ClientGame::GetAction action_ {}, in: {}", status_, choice);
    if (status_ == WAITING) continue; // Skip as long as not active. 
    if (status_ == CLOSING) break; // Leave thread.

    // Throw event
    if (contexts_.at(current_context_).eventmanager().handlers().count(choice) > 0) {
      spdlog::get(LOGGER)->debug("ClientGame::GetAction: calling handler.");
      spdlog::get(LOGGER)->flush();
      (this->*contexts_.at(current_context_).eventmanager().handlers().at(choice))(0);
    }
    // If event not in context-event-manager print availible options.
    else {
      spdlog::get(LOGGER)->debug("ClientGame::GetAction: invalid action for this context.");
      spdlog::get(LOGGER)->flush();
      drawrer_.set_msg("Invalid option. Availible: " + contexts_.at(current_context_).eventmanager().options());
    }

    // Refresh field (only side-column)
    if (current_context_ == CONTEXT_FIELD)
      drawrer_.PrintGame(false, false); 
    else 
      drawrer_.PrintGame(false, true); 
  }

  // Send server message to close game.
  nlohmann::json response = {{"command", "close"}, {"username", username_}, {"data", nlohmann::json()}};
  ws_srv_->SendMessage(response.dump());

  // Wrap up.
  refresh();
  clear();
  endwin();
}

void ClientGame::h_Quit(int) {
  drawrer_.ClearField();
  drawrer_.set_stop_render(true);
  drawrer_.PrintCenteredLine(LINES/2, "Are you sure you want to quit? (y/n)");
  char choice = getch();
  if (choice == 'y') {
    status_ = CLOSING;
    nlohmann::json msg = {{"command", "resign"}, {"username", username_}, {"data", nlohmann::json()}};
    ws_srv_->SendMessage(msg.dump());
  }
  else {
    drawrer_.set_stop_render(false);
  }
}

void ClientGame::h_MoveSelectionUp(int) {
  drawrer_.inc_cur_sidebar_elem(1);
}

void ClientGame::h_MoveSelectionDown(int) {
  drawrer_.inc_cur_sidebar_elem(-1);
}

void ClientGame::h_MoveSelectionLeft(int) {
  drawrer_.inc_cur_sidebar_elem(-2);
}

void ClientGame::h_MoveSelectionRight(int) {
  drawrer_.inc_cur_sidebar_elem(2);
}

void ClientGame::h_ChangeViewPoint(int) {
  current_context_ = drawrer_.next_viewpoint();
  drawrer_.set_msg(contexts_.at(current_context_).msg());
  drawrer_.PrintTopline(contexts_.at(current_context_).topline());
}

void ClientGame::h_AddIron(int) {
  int resource = drawrer_.GetResource();
  nlohmann::json response = {{"command", "add_iron"}, {"username", username_}, {"data", 
    {{"resource", resource}} }};
  ws_srv_->SendMessage(response.dump());
}

void ClientGame::h_RemoveIron(int) {
  int resource = drawrer_.GetResource();
  nlohmann::json response = {{"command", "remove_iron"}, {"username", username_}, {"data", 
    {{"resource", resource}} }};
  ws_srv_->SendMessage(response.dump());
}

void ClientGame::h_AddTech(int) {
  int technology = drawrer_.GetTech();
  nlohmann::json response = {{"command", "add_technology"}, {"username", username_}, {"data", 
    {{"technology", technology}} }};
  ws_srv_->SendMessage(response.dump());
}

void ClientGame::h_BuildActivatedNeuron(int) {
  nlohmann::json response = {{"command", "check_build_neuron"}, {"username", username_}, {"data",
    {{"unit", ACTIVATEDNEURON}} }};
  ws_srv_->SendMessage(response.dump());
}

void ClientGame::h_BuildSynapse(int) {
  nlohmann::json response = {{"command", "check_build_neuron"}, {"username", username_}, {"data",
    {{"unit", SYNAPSE}} }};
  ws_srv_->SendMessage(response.dump());
}

void ClientGame::h_BuildEpsp(int) {
  int num = (history_.size() > 1 && std::isdigit(history_[history_.size()-2])) 
    ? history_[history_.size()-2] - '0' : 1;
  BuildPotential({-1, -1}, EPSP, num);
}

void ClientGame::h_BuildIpsp(int) {
  int num = (history_.size() > 1 && std::isdigit(history_[history_.size()-2])) 
    ? history_[history_.size()-2] - '0' : 1;
  BuildPotential({-1, -1}, IPSP, num);
}

void ClientGame::h_BuildNeuron(int) {
  int unit = contexts_.at(current_context_).current_unit();
  nlohmann::json response = {{"command", "build_neuron"}, {"username", username_}, {"data",
    {{"unit", unit}, {"pos", drawrer_.field_pos()}} }};
  ws_srv_->SendMessage(response.dump());
}

void ClientGame::h_SelectNeuron(int) {
  position_t pos = drawrer_.GetMarkerPos(utils::CharToString(history_.back(), 0));
  if (pos.first != -1) {
    int unit = contexts_[current_context_].current_unit();
    // For potential, simply send command to build potential
    if (unit == IPSP || unit == EPSP) {
      int num = (history_.size() > 1 && std::isdigit(history_[history_.size()-3])) 
        ? history_[history_.size()-3]-'0' : 1;
      BuildPotential(pos, unit, num);
      current_context_ = CONTEXT_RESOURCES;
      drawrer_.set_viewpoint(current_context_);
    }
    // For neurons, continue selection process.
    else 
      SetUpFieldPositionSelect(pos, unit, contexts_[current_context_].current_range());
    contexts_.erase(CONTEXT_NEURON_SELECT);
    drawrer_.ClearMarkers();
  }
  else
    drawrer_.set_msg("Invalid position selected.");
}

void ClientGame::h_SelectSynapse(int) {
  position_t pos = drawrer_.GetMarkerPos(utils::CharToString(history_.back(), 0));
  if (pos.first != -1) {
    nlohmann::json msg = {{"data", {{"context", CONTEXT_SYNAPSE}, {"pos", pos}} }};
    m_ChangeContext(msg);
  }
  else
    drawrer_.set_msg("Invalid position selected.");
}

void ClientGame::h_SendSelectSynapse(int) {
  nlohmann::json response = {{"command", "select_synapse"}, {"username", username_}, {"data",
    nlohmann::json()}};
  ws_srv_->SendMessage(response.dump());
}

void ClientGame::h_SetWP(int) {
  nlohmann::json response = {{"command", "set_way_point"}, {"username", username_}, {"data",
    {{"pos", contexts_.at(current_context_).current_pos()}} }};
  ws_srv_->SendMessage(response.dump());
}

void ClientGame::h_AddWP(int) {
  nlohmann::json response = {{"command", "add_way_point"}, {"username", username_}, {"data",
    {{"pos", contexts_.at(current_context_).current_pos()}} }};
  ws_srv_->SendMessage(response.dump());
}

void ClientGame::h_EpspTarget(int) {
  nlohmann::json response = {{"command", "set_epsp_target"}, {"username", username_}, {"data",
    {{"pos", contexts_.at(current_context_).current_pos()}} }};
  ws_srv_->SendMessage(response.dump());
}

void ClientGame::h_IpspTarget(int) {
  nlohmann::json response = {{"command", "set_ipsp_target"}, {"username", username_}, {"data",
    {{"pos", contexts_.at(current_context_).current_pos()}} }};
  ws_srv_->SendMessage(response.dump());
}

void ClientGame::h_SwarmAttack(int) {
  nlohmann::json response = {{"command", "toggle_swarm_attack"}, {"username", username_}, {"data",
    {{"pos", contexts_.at(current_context_).current_pos()}} }};
  ws_srv_->SendMessage(response.dump());
}

void ClientGame::BuildPotential(position_t pos, int unit, int num) {
  nlohmann::json response = {{"command", "check_build_potential"}, {"username", username_}, {"data", {
    {"unit", unit}, {"num", num}} }};
  drawrer_.set_msg("Requested " + std::to_string(num) + " potentials.");
  if (pos.first != -1 && pos.second != -1)
    response["data"]["pos"] = pos;
  ws_srv_->SendMessage(response.dump());
}

void ClientGame::m_SelectMode(nlohmann::json& msg) {
  spdlog::get(LOGGER)->debug("ClientGame::m_SelectMode: {}", msg.dump());
  // Print welcome text.
  drawrer_.PrintCenteredParagraphs(texts::welcome);
  
  // Update msg
  msg["command"] = "init_game";
  msg["data"] = {{"lines", drawrer_.field_height()}, {"cols", drawrer_.field_width()}, {"base_path", base_path_}};
 
  // Select single-player, mulit-player (host/ client), observer.
  choice_mapping_t mapping = {
    {SINGLE_PLAYER, {"singe-player", COLOR_AVAILIBLE}}, 
    {MULTI_PLAYER, {"muli-player (host)", (muliplayer_availible_) ? COLOR_AVAILIBLE : COLOR_DEFAULT}}, 
    {MULTI_PLAYER_CLIENT, {"muli-player (client)", (muliplayer_availible_) ? COLOR_AVAILIBLE : COLOR_DEFAULT}}, 
    {OBSERVER, {"watch ki", COLOR_DEFAULT}}
  };
  msg["data"]["mode"] = SelectInteger("Select mode", true, mapping, {mapping.size()+1}, "Mode not available");
  // If host, then also select number of players.
  if (msg["data"]["mode"] == MULTI_PLAYER) {
    mapping = {
      {0, {"2 players", COLOR_AVAILIBLE}}, 
      {1, {"3 players", (muliplayer_availible_) ? COLOR_AVAILIBLE : COLOR_DEFAULT}}, 
      {2, {"4 players", (muliplayer_availible_) ? COLOR_AVAILIBLE : COLOR_DEFAULT}}, 
    };
    msg["data"]["num_players"] = 2 + SelectInteger("Select number of players", true, 
        mapping, {mapping.size()+1}, "Max 4 players!");
  }
}

void ClientGame::m_SelectAudio(nlohmann::json& msg) {
  msg["command"] = "initialize_game";
  msg["data"]["source_path"] = SelectAudio();
}

void ClientGame::m_PrintMsg(nlohmann::json& msg) {
  drawrer_.ClearField();
  drawrer_.PrintCenteredLine(LINES/2, msg["data"]["msg"]);
  refresh();
  msg = nlohmann::json();
}

void ClientGame::m_InitGame(nlohmann::json& msg) {
  drawrer_.set_transfer(msg["data"]);
  drawrer_.PrintGame(false, false);
  status_ = RUNNING;
  drawrer_.set_msg(contexts_.at(current_context_).msg());
  drawrer_.PrintTopline(contexts_.at(current_context_).topline());
  msg = nlohmann::json();
}

void ClientGame::m_UpdateGame(nlohmann::json& msg) {
  drawrer_.UpdateTranser(msg["data"]);
  drawrer_.PrintGame(false, false);
  msg = nlohmann::json();
}

void ClientGame::m_SetMsg(nlohmann::json& msg) {
  drawrer_.set_msg(msg["data"]["msg"]);
  msg = nlohmann::json();
}

void ClientGame::m_GameEnd(nlohmann::json& msg) {
  status_ = CLOSING;
  drawrer_.ClearField();
  drawrer_.PrintCenteredLine(LINES/2, msg["data"]["msg"]);
  getch();
  msg = nlohmann::json();
}

void ClientGame::m_SelectPosition(nlohmann::json& msg) {
  std::vector<position_t> positions = msg["data"]["positions"];
  int unit = msg["data"]["unit"];
  int range = msg["data"].value("range", 1);
  if (positions.size() == 1)
    SetUpFieldPositionSelect(positions.front(), unit, range);
  else
    SetUpFieldNeuronSelect(positions, unit, range);
  msg = nlohmann::json();
}

void ClientGame::m_SelectPositionSynapse(nlohmann::json& msg) {
  spdlog::get(LOGGER)->debug("ClientGame::m_SelectPositionSynapse: {}", msg.dump());
  spdlog::get(LOGGER)->flush();
  std::vector<position_t> positions = msg["data"]["positions"];

  std::map<char, void(ClientGame::*)(int)> new_handlers = {{'t', &ClientGame::h_ChangeViewPoint}};
  for (unsigned int i=0; i<positions.size(); i++) {
    new_handlers['a'+i] = &ClientGame::h_SelectSynapse;
    drawrer_.AddMarker(positions[i], utils::CharToString('a', i), COLOR_AVAILIBLE);
  }
  Context context = Context("Select neuron", new_handlers, {{" [a, b, ...] to select neuron ", 
      COLOR_DEFAULT}, {" [h]elp ", COLOR_DEFAULT}, {" [q]uit ", COLOR_DEFAULT}});
  current_context_ = CONTEXT_NEURON_SELECT;
  contexts_[current_context_] = context;
  drawrer_.PrintTopline(contexts_.at(current_context_).topline());
  msg = nlohmann::json();
}

void ClientGame::m_SetUnit(nlohmann::json& msg) {
  drawrer_.AddNewUnitToPos(msg["data"]["pos"], msg["data"]["unit"], msg["data"]["color"]);
  current_context_ = CONTEXT_RESOURCES;
  drawrer_.PrintTopline(contexts_.at(current_context_).topline());
  drawrer_.set_viewpoint(current_context_);
  drawrer_.PrintGame(false, false);
  drawrer_.set_msg("Success!");
}

void ClientGame::m_SetUnits(nlohmann::json& msg) {
  std::map<position_t, int> neurons = msg["data"]["neurons"];
  int color = msg["data"]["color"];
  for (const auto& it : neurons) 
    drawrer_.AddNewUnitToPos(it.first, it.second, color);
  drawrer_.PrintGame(false, false);
}

void ClientGame::m_ChangeContext(nlohmann::json& msg) {
  spdlog::get(LOGGER)->debug("ClientGame::m_ChangeContext");
  current_context_ = msg["data"]["context"];
  spdlog::get(LOGGER)->debug("ClientGame::m_ChangeContext: changed context to {}", current_context_);
  // Add data to context.
  if (msg["data"].contains("pos"))
    contexts_.at(current_context_).set_current_pos(utils::PositionFromVector(msg["data"]["pos"]));
  if (msg["data"].contains("range"))
    contexts_.at(current_context_).set_current_range(msg["data"]["range"]);
  if (msg["data"].contains("unit"))
    contexts_.at(current_context_).set_current_unit(msg["data"]["unit"]);
  drawrer_.PrintTopline(contexts_.at(current_context_).topline());
  drawrer_.set_viewpoint(current_context_);
  drawrer_.PrintGame(false, false);
  drawrer_.set_msg(contexts_.at(current_context_).msg());
}

// Selection methods

void ClientGame::SetUpFieldPositionSelect(position_t pos, int unit, int range) {
  current_context_ = CONTEXT_FIELD;
  contexts_.at(current_context_).set_current_unit(unit);
  drawrer_.PrintTopline(contexts_.at(current_context_).topline());
  drawrer_.set_viewpoint(CONTEXT_FIELD);
  drawrer_.set_field_start_pos(pos);
  drawrer_.set_range({pos, range});
  drawrer_.set_msg("Select position where to build " + units_tech_name_mapping.at(unit) 
      + " (use [t] to return to normal mode)");
}

void ClientGame::SetUpFieldNeuronSelect(std::vector<position_t> positions, int unit, int range) {
  std::map<char, void(ClientGame::*)(int)> new_handlers = {{'t', &ClientGame::h_ChangeViewPoint}};
  for (unsigned int i=0; i<positions.size(); i++) {
    new_handlers['a'+i] = &ClientGame::h_SelectNeuron;
    drawrer_.AddMarker(positions[i], utils::CharToString('a', i), COLOR_AVAILIBLE);
  }
  Context context = Context("Select neuron", new_handlers, {{" [a, b, ...] to select neuron ", 
      COLOR_DEFAULT}, {" [h]elp ", COLOR_DEFAULT}, {" [q]uit ", COLOR_DEFAULT}});
  context.set_current_unit(unit);
  context.set_current_range(range);
  current_context_ = CONTEXT_NEURON_SELECT;
  contexts_[current_context_] = context;
  drawrer_.PrintTopline(contexts_.at(current_context_).topline());
}

int ClientGame::SelectInteger(std::string msg, bool omit, choice_mapping_t& mapping, std::vector<size_t> splits,
    std::string error_msg) {
  drawrer_.ClearField();
  bool end = false;

  std::vector<std::pair<std::string, int>> options;
  for (const auto& option : mapping) {
    options.push_back({utils::CharToString('a', option.first) + ": " + option.second.first + "    ", 
        option.second.second});
  }
  
  // Print matching the splits.
  int counter = 0;
  int last_split = 0;
  for (const auto& split : splits) {
    std::vector<std::pair<std::string, int>> option_part; 
    for (unsigned int i=last_split; i<split && i<options.size(); i++)
      option_part.push_back(options[i]);
    drawrer_.PrintCenteredLineColored(LINES/2+(counter+=2), option_part);
    last_split = split;
  }
  drawrer_.PrintCenteredLine(LINES/2-1, msg);
  drawrer_.PrintCenteredLine(LINES/2+counter+3, "> enter number...");

  while (!end) {
    // Get choice.
    char choice = getch();
    int int_choice = choice-'a';
    if (choice == 'q' && omit)
      end = true;
    else if (mapping.count(int_choice) > 0 && (mapping.at(int_choice).second == COLOR_AVAILIBLE || !omit)) {
      // TODO(fux): see above  `pause_ = false;`
      return int_choice;
    }
    else if (mapping.count(int_choice) > 0 && mapping.at(int_choice).second != COLOR_AVAILIBLE && omit)
      drawrer_.PrintCenteredLine(LINES/2+counter+5, "Selection not available (" + error_msg + "): " 
          + std::to_string(int_choice));
    else 
      drawrer_.PrintCenteredLine(LINES/2+counter+5, "Wrong selection: " + std::to_string(int_choice));
  }
  // TODO(fux): see above  `pause_ = false;`
  return -1;
}

std::string ClientGame::SelectAudio() {
  drawrer_.ClearField();

  // Create selector and define some variables
  AudioSelector selector = SetupAudioSelector("", "select audio", audio_paths_);
  selector.options_.push_back({"dissonance_recently_played", "recently played"});
  std::vector<std::string> recently_played = utils::LoadJsonFromDisc(base_path_ + "/settings/recently_played.json");
  std::string error = "";
  std::string help = "(use + to add paths, ENTER to select,  h/l or ←/→ to change directory "
    "and j/k or ↓/↑ to circle through songs,)";
  unsigned int selected = 0;
  int level = 0;
  unsigned int print_start = 0;
  unsigned int max = LINES/2;
  std::vector<std::pair<std::string, std::string>> visible_options;

  while(true) {
    unsigned int print_max = std::min((unsigned int)selector.options_.size(), max);
    visible_options = utils::SliceVector(selector.options_, print_start, print_max);
    
    drawrer_.PrintCenteredLine(10, utils::ToUpper(selector.title_));
    drawrer_.PrintCenteredLine(11, selector.path_);
    drawrer_.PrintCenteredLine(12, help);

    attron(COLOR_PAIR(COLOR_ERROR));
    drawrer_.PrintCenteredLine(13, error);
    error = "";
    attron(COLOR_PAIR(COLOR_DEFAULT));

    for (unsigned int i=0; i<visible_options.size(); i++) {
      if (i == selected)
        attron(COLOR_PAIR(COLOR_MARKED));
      drawrer_.PrintCenteredLine(15 + i, visible_options[i].second);
      attron(COLOR_PAIR(COLOR_DEFAULT));
    }

    // Get players choice.
    char choice = getch();
    if (utils::IsRight(choice)) {
      level++;
      if (visible_options[selected].first == "dissonance_recently_played")
        selector = SetupAudioSelector("", "Recently Played", recently_played);
      else if (std::filesystem::is_directory(visible_options[selected].first)) {
        selector = SetupAudioSelector(visible_options[selected].first, visible_options[selected].second, 
            utils::GetAllPathsInDirectory(visible_options[selected].first));
        selected = 0;
        print_start = 0;
      }
      else 
        error = "Not a directory!";
    }
    else if (utils::IsLeft(choice)) {
      if (level == 0) 
        error = "No parent directory.";
      else {
        level--;
        selected = 0;
        print_start = 0;
        if (level == 0) {
          selector = SetupAudioSelector("", "select audio", audio_paths_);
          selector.options_.push_back({"dissonance_recently_played", "recently played"});
        }
        else {
          std::filesystem::path p = selector.path_;
          selector = SetupAudioSelector(p.parent_path().string(), p.parent_path().filename().string(), 
              utils::GetAllPathsInDirectory(p.parent_path()));
        }
      }
    }
    else if (utils::IsDown(choice)) {
      if (selected == print_max-1 && selector.options_.size() > max)
        print_start++;
      else 
        selected = utils::Mod(selected+1, visible_options.size());
    }
    else if (utils::IsUp(choice)) {
      if (selected == 0 && print_start > 0)
        print_start--;
      else 
        selected = utils::Mod(selected-1, print_max);
    }
    else if (std::to_string(choice) == "10") {
      std::filesystem::path select_path = visible_options[selected].first;
      if (select_path.filename().extension() == ".mp3" || select_path.filename().extension() == ".wav")
        break;
      else 
        error = "Wrong file type. Select mp3 or wav";
    }
    else if (choice == '+') {
      std::string input = InputString("Absolute path: ");
      if (std::filesystem::exists(input)) {
        if (input.back() == '/')
          input.pop_back();
        audio_paths_.push_back(input);
        nlohmann::json audio_paths = utils::LoadJsonFromDisc(base_path_ + "/settings/music_paths.json");
        audio_paths.push_back(input);
        utils::WriteJsonFromDisc(base_path_ + "/settings/music_paths.json", audio_paths);
        selector = SetupAudioSelector("", "select audio", audio_paths_);
      }
      else {
        error = "Path does not exist.";
      }
    }
    drawrer_.ClearField();
  }

  // Add selected aubio-file to recently-played files.
  std::filesystem::path select_path = selector.options_[selected].first;
  bool exists=false;
  for (const auto& it : recently_played) {
    if (it == select_path.string()) {
      exists = true;
      break;
    }
  }
  if (!exists)
    recently_played.push_back(select_path.string());
  if (recently_played.size() > 10) 
    recently_played.erase(recently_played.begin());
  nlohmann::json j_recently_played = recently_played;
  utils::WriteJsonFromDisc(base_path_ + "/settings/recently_played.json", j_recently_played);

  // Return selected path.
  return select_path.string(); 
}

ClientGame::AudioSelector ClientGame::SetupAudioSelector(std::string path, std::string title, 
    std::vector<std::string> paths) {
  std::vector<std::pair<std::string, std::string>> options;
  for (const auto& it : paths) {
    std::filesystem::path path = it;
    if (path.extension() == ".mp3" || path.extension() == ".wav" || std::filesystem::is_directory(path))
      options.push_back({it, path.filename()});
  }
  return AudioSelector({path, title, options});
}

std::string ClientGame::InputString(std::string instruction) {
  drawrer_.ClearField();
  drawrer_.PrintCenteredLine(LINES/2, instruction.c_str());
  echo();
  curs_set(1);
  std::string input;
  int ch = getch();
  while (ch != '\n') {
    input.push_back(ch);
    ch = getch();
  }
  noecho();
  curs_set(0);
  return input;
}

