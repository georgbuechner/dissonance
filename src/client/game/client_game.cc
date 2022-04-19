#include "client/game/client_game.h"
#include "client/game/print/drawrer.h"
#include "client/websocket/client.h"
#include "client_game.h"
#include "curses.h"
#include "nlohmann/json_fwd.hpp"
#include "share/audio/audio.h"
#include "share/constants/codes.h"
#include "share/constants/texts.h"
#include "share/defines.h"
#include "share/objects/dtos.h"
#include "share/objects/units.h"
#include "share/objects/transfer.h"
#include "share/tools/context.h"
#include "share/tools/eventmanager.h"
#include "share/tools/utils/utils.h"

#include "spdlog/spdlog.h"

#include <cctype>
#include <filesystem>
#include <iterator>
#include <math.h>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unistd.h>
#include <utility>
#include <vector>

#define STD_HANDLERS -1

#define CONTEXT_RESOURCES_MSG "Distribute (+)/ remove (-) iron to handler resource-gain"
#define CONTEXT_TECHNOLOGIES_MSG "Research technology by pressing [enter]"
#define CONTEXT_SYNAPSE_MSG "Select instructions for this synapse"

std::map<int, std::map<char, void(ClientGame::*)(nlohmann::json&)>> ClientGame::handlers_ = {};

void ClientGame::init(){
  handlers_ = {
    { STD_HANDLERS, {
        {'j', &ClientGame::h_MoveSelectionUp}, {'k', &ClientGame::h_MoveSelectionDown}, 
        {'t', &ClientGame::h_ChangeViewPoint}, {'q', &ClientGame::h_Quit}, {'s', &ClientGame::h_SendSelectSynapse}, 
        {'A', &ClientGame::h_BuildNeuron}, {'S', &ClientGame::h_BuildNeuron}, {'N', &ClientGame::h_BuildNeuron}, 
        {'e', &ClientGame::h_BuildPotential}, {'i', &ClientGame::h_BuildPotential}, 
        {'m', &ClientGame::h_BuildPotential}, {'?', &ClientGame::h_Help},
      }
    },
    { CONTEXT_FIELD, {
        {'h', &ClientGame::h_MoveSelectionLeft}, {'l', &ClientGame::h_MoveSelectionRight}, 
        {'\n', &ClientGame::h_AddPosition}, {'q', &ClientGame::h_ToResourceContext},
      }
    },
    { CONTEXT_RESOURCES, {{'+', &ClientGame::h_AddIron}, {'-', &ClientGame::h_RemoveIron}} },
    { CONTEXT_TECHNOLOGIES, {{'\n', &ClientGame::h_AddTech}} },
    { CONTEXT_SYNAPSE, {
        {'s', &ClientGame::h_SetWPs}, {'i', &ClientGame::h_IpspTarget}, {'e', &ClientGame::h_EpspTarget}, 
        {'w', &ClientGame::h_SwarmAttack}, {'t', &ClientGame::h_ChangeViewPoint}, 
        {'q', &ClientGame::h_ResetOrQuitSynapseContext}
      }
    },
    { CONTEXT_POST_GAME, {{'h', &ClientGame::h_MoveSelectionUp}, {'l', &ClientGame::h_MoveSelectionDown}}},
    { CONTEXT_LOBBY, 
      {
        {'k', &ClientGame::h_MoveSelectionUp}, {'j', &ClientGame::h_MoveSelectionDown},
        {'\n', &ClientGame::h_SelectGame}
      }
    },
    { CONTEXT_TEXT,
      {
        {'h', &ClientGame::h_TextPrev}, {'l', &ClientGame::h_TextNext},
        {'q', &ClientGame::h_TextQuit}, 
      }
    }
  };
}

// std-topline
t_topline std_topline = {{"[i]psp (a..z)  ", COLOR_DEFAULT}, {"[e]psp (1..9)  ", COLOR_DEFAULT}, 
  {"[m]akro (0)  ", COLOR_DEFAULT}, {"[A]ctivated Neuron (" SYMBOL_DEF ")  ", COLOR_DEFAULT}, 
  {"[S]ynape (" SYMBOL_BARACK ")  ", COLOR_DEFAULT}, {"[N]ucleus (" SYMBOL_DEN ")  ", COLOR_DEFAULT}, 
  {" [s]elect-synapse ", COLOR_DEFAULT}, {" [t]oggle-navigation  ", COLOR_DEFAULT}, {" [h]elp  ", COLOR_DEFAULT}, 
  {" [q]uit  ", COLOR_DEFAULT}};

t_topline field_topline = {{" [h, j, k, l] to navigate field " " [t]oggle-navigation ", 
  COLOR_DEFAULT}, {" [h]elp ", COLOR_DEFAULT}, {" [q]uit ", COLOR_DEFAULT}};

t_topline synapse_topline = {{" [s]et way-points ", COLOR_DEFAULT}, {" [i]psp-target ", 
  COLOR_DEFAULT}, { " [e]psp-target ", COLOR_DEFAULT}, { " toggle s[w]arm-attack ", COLOR_DEFAULT},
  {" [t]oggle-navigation ", COLOR_DEFAULT}, {" [h]elp ", COLOR_DEFAULT}, {" [q]uit ", COLOR_DEFAULT}
};

ClientGame::ClientGame(std::string base_path, std::string username, bool mp) : username_(username), 
    muliplayer_availible_(mp), base_path_(base_path), render_pause_(false), drawrer_(), audio_(base_path) {
  status_ = WAITING;

  // apply standard settings:
  LoadSettings();

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
  // init_pair(COLOR_AVAILIBLE, COLOR_BLUE, -1);
  init_pair(COLOR_AVAILIBLE, COLOR_BLUE, -1);
  init_pair(COLOR_ERROR, COLOR_RED, -1);
  init_pair(COLOR_DEFAULT, -1, -1);
  init_pair(COLOR_MSG, COLOR_CYAN, -1);
  init_pair(COLOR_SUCCESS, COLOR_GREEN, -1);
  init_pair(COLOR_MARKED, COLOR_MAGENTA, -1);

  init_pair(COLOR_P2, 10, -1); // player 
  init_pair(COLOR_P3, 11, -1); // player 
  init_pair(COLOR_P4, 12, -1); // player 
  init_pair(COLOR_P5, 13, -1); // player

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
  if (!muliplayer_availible_) {
    handlers_[STD_HANDLERS][' '] = &ClientGame::h_PauseAndUnPause;
    std_topline.push_back({"[space] pause", COLOR_AVAILIBLE});
  }

  current_context_ = CONTEXT_RESOURCES;
  contexts_[CONTEXT_FIELD] = Context("", handlers_[STD_HANDLERS], handlers_[CONTEXT_FIELD], field_topline);
  contexts_[CONTEXT_RESOURCES] = Context(CONTEXT_RESOURCES_MSG, handlers_[STD_HANDLERS], handlers_[CONTEXT_RESOURCES], 
      std_topline);
  contexts_[CONTEXT_TECHNOLOGIES] = Context(CONTEXT_TECHNOLOGIES_MSG, handlers_[STD_HANDLERS], 
      handlers_[CONTEXT_TECHNOLOGIES], std_topline);
  contexts_[CONTEXT_SYNAPSE] = Context(CONTEXT_SYNAPSE_MSG, handlers_[CONTEXT_SYNAPSE], synapse_topline);
  contexts_[CONTEXT_POST_GAME] = Context("", handlers_[CONTEXT_POST_GAME], std_topline);
  contexts_[CONTEXT_LOBBY] = Context("", handlers_[CONTEXT_LOBBY], std_topline);
  contexts_[CONTEXT_TEXT] = Context("", handlers_[CONTEXT_TEXT], std_topline);

  // Initialize eventmanager.
  eventmanager_.AddHandler("kill", &ClientGame::h_Kill);
  eventmanager_.AddHandler("select_mode", &ClientGame::m_SelectMode);
  eventmanager_.AddHandler("select_audio", &ClientGame::m_SelectAudio);
  eventmanager_.AddHandler("print_msg", &ClientGame::m_PrintMsg);
  eventmanager_.AddHandler("init_game", &ClientGame::m_InitGame);
  eventmanager_.AddHandler("update_game", &ClientGame::m_UpdateGame);
  eventmanager_.AddHandler("update_lobby", &ClientGame::m_UpdateLobby);
  eventmanager_.AddHandler("set_msg", &ClientGame::m_SetMsg);
  eventmanager_.AddHandler("game_end", &ClientGame::m_GameEnd);
  eventmanager_.AddHandler("set_unit", &ClientGame::m_SetUnit);
  eventmanager_.AddHandler("set_units", &ClientGame::m_SetUnits);
  eventmanager_.AddHandler("build_neuron", &ClientGame::h_BuildNeuron);
  eventmanager_.AddHandler("build_potential", &ClientGame::h_BuildPotential);
  eventmanager_.AddHandler("select_synapse", &ClientGame::h_SendSelectSynapse);
  eventmanager_.AddHandler("set_wps", &ClientGame::h_SetWPs);
  eventmanager_.AddHandler("ipsp_target", &ClientGame::h_IpspTarget);
  eventmanager_.AddHandler("epsp_target", &ClientGame::h_EpspTarget);

  eventmanager_tutorial_.AddHandler("init_game", &ClientGame::h_TutorialGetOxygen);
  eventmanager_tutorial_.AddHandler("set_unit", &ClientGame::h_TutorialSetUnit);
  eventmanager_tutorial_.AddHandler("set_units", &ClientGame::h_TutorialScouted);
  eventmanager_tutorial_.AddHandler("build_neuron", &ClientGame::h_TutorialBuildNeuron);
  eventmanager_tutorial_.AddHandler("set_msg", &ClientGame::h_TutorialSetMessage);
  eventmanager_tutorial_.AddHandler("update_game", &ClientGame::h_TutorialUpdateGame);

  tutorial_ = Tutorial({0, 0, 0, true, true, false, false, false, false, false, false, false, false, false});
  drawrer_.CreateMiniFields((username_.front() > 'A') ? COLOR_P3 : COLOR_P2);
}

void ClientGame::HandleAction(nlohmann::json msg) {
  std::string command = msg["command"];
  spdlog::get(LOGGER)->debug("ClientGame::HandleAction: {}", command);

  nlohmann::json original_message = msg;

  // Get event from event-manager
  if (eventmanager_.handlers().count(command))
    (this->*eventmanager_.handlers().at(command))(msg);
  else 
    msg = nlohmann::json(); // if no matching event set return-message to empty.
  if (mode_ == TUTORIAL && eventmanager_tutorial_.handlers().count(command) > 0) {
    (this->*eventmanager_tutorial_.handlers().at(command))(original_message);
  }
  
  spdlog::get(LOGGER)->debug("ClientGame::HandleAction: response {}", msg.dump());
  if (!msg.contains("username"))
    msg["username"] = username_;
  if (msg.contains("command"))
    ws_srv_->SendMessage(msg.dump());
}

void ClientGame::GetAction() {
  spdlog::get(LOGGER)->info("ClientGame::GetAction stated");

  while(true) {
    // Get Input
    if (status_ == WAITING) continue; // Skip as long as not active.
    char choice = getch();
    history_.push_back(choice);
    spdlog::get(LOGGER)->info("ClientGame::GetAction action_ {}, in: {}", status_, choice);
    if (status_ == WAITING)
      continue; // Skip as long as not active. 

    if (current_context_ == CONTEXT_POST_GAME && choice == 'q')
      break;

    // Throw event
    std::shared_lock sl(mutex_context_);
    if (contexts_.at(current_context_).eventmanager().handlers().count(choice) > 0) {
      spdlog::get(LOGGER)->debug("ClientGame::GetAction: calling handler.");
      nlohmann::json data = contexts_.at(current_context_).data();
      contexts_.at(current_context_).set_cmd(choice);
      sl.unlock();
      (this->*contexts_.at(current_context_).eventmanager().handlers().at(choice))(data);
    }
    else if (mode_ == TUTORIAL && tutorial_.action_active_ && choice == 'y') {
      nlohmann::json data;
      h_TutorialAction(data);
    }
    // If event not in context-event-manager print availible options.
    else {
      spdlog::get(LOGGER)->debug("ClientGame::GetAction: invalid action for this context.");
      drawrer_.set_msg("Invalid option. Availible: " + contexts_.at(current_context_).eventmanager().options());
    }

    // Refresh field (only side-column)
    if (current_context_ == CONTEXT_FIELD)
      drawrer_.PrintGame(false, false, current_context_); 
    // Lobby
    else if (current_context_ == CONTEXT_LOBBY)
      drawrer_.PrintLobby();
    // Postgame print statistics
    else if (current_context_ == CONTEXT_POST_GAME)
      drawrer_.PrintStatistics();
    // Normal update
    else 
      drawrer_.PrintGame(false, true, current_context_); 
  }

  // Wrap up and exit.
  WrapUp();
}

void ClientGame::h_Kill(nlohmann::json& msg) {
  drawrer_.set_stop_render(true);
  drawrer_.ClearField();
  drawrer_.PrintCenteredLine(LINES/2, msg["data"]["msg"].get<std::string>() 
      + " [Press any key to leave game]");
  refresh();
  getch();
  // Wrap up and exit.
  WrapUp();
}

void ClientGame::h_Help(nlohmann::json&) {
  if (mode_ == TUTORIAL)
    Pause();
  drawrer_.set_stop_render(true);
  drawrer_.PrintCenteredParagraphs(texts::help);
  drawrer_.set_stop_render(false);
  if (mode_ == TUTORIAL)
    UnPause();
}

void ClientGame::h_Quit(nlohmann::json&) {
  spdlog::get(LOGGER)->debug("ClientGame::h_Quit");
  drawrer_.ClearField();
  drawrer_.set_stop_render(true);
  drawrer_.PrintCenteredLine(LINES/2, "Are you sure you want to quit? (y/n)");
  char choice = getch();
  if (choice == 'y') {
    // Wrap up and exit.
    WrapUp();
  }
  else {
    drawrer_.set_stop_render(false);
  }
}

void ClientGame::h_PauseAndUnPause(nlohmann::json&) {
  if (pause_)
    UnPause();
  else 
    Pause();
}

void ClientGame::h_MoveSelectionUp(nlohmann::json&) {
  drawrer_.inc_cur_sidebar_elem(1);
}

void ClientGame::h_MoveSelectionDown(nlohmann::json&) {
  drawrer_.inc_cur_sidebar_elem(-1);
}

void ClientGame::h_MoveSelectionLeft(nlohmann::json&) {
  drawrer_.inc_cur_sidebar_elem(-2);
}

void ClientGame::h_MoveSelectionRight(nlohmann::json&) {
  drawrer_.inc_cur_sidebar_elem(2);
}

void ClientGame::h_SelectGame(nlohmann::json&) {
  spdlog::get(LOGGER)->info("ClientGame::h_SelectGame");
  nlohmann::json msg;
  msg["username"] = username_;
  msg["command"] = "init_game";
  msg["data"] = {{"lines", drawrer_.field_height()}, {"cols", drawrer_.field_width()}, 
    {"base_path", base_path_}, {"mode", MULTI_PLAYER_CLIENT}};
  std::string game_id = drawrer_.game_id_from_lobby();
  if (game_id != "") {
    msg["data"]["game_id"] = drawrer_.game_id_from_lobby();
    spdlog::get(LOGGER)->info("ClientGame::h_SelectGame: {}", msg.dump());
    ws_srv_->SendMessage(msg.dump());
    current_context_ = CONTEXT_RESOURCES;
    drawrer_.set_viewpoint(current_context_);
  }
  else {
    drawrer_.set_msg("Game probably no longer availible");
  }
}

void ClientGame::h_ChangeViewPoint(nlohmann::json&) {
  drawrer_.ClearMarkers();
  current_context_ = drawrer_.next_viewpoint();
  drawrer_.set_msg(contexts_.at(current_context_).msg());
  drawrer_.set_topline(contexts_.at(current_context_).topline());
}

void ClientGame::h_AddIron(nlohmann::json&) {
  int resource = drawrer_.GetResource();
  nlohmann::json response = {{"command", "add_iron"}, {"username", username_}, {"data", 
    {{"resource", resource}} }};
  ws_srv_->SendMessage(response.dump());
}

void ClientGame::h_RemoveIron(nlohmann::json&) {
  int resource = drawrer_.GetResource();
  nlohmann::json response = {{"command", "remove_iron"}, {"username", username_}, {"data", 
    {{"resource", resource}} }};
  ws_srv_->SendMessage(response.dump());
}

void ClientGame::h_AddTech(nlohmann::json&) {
  int technology = drawrer_.GetTech();
  nlohmann::json response = {{"command", "add_technology"}, {"username", username_}, {"data", 
    {{"technology", technology}} }};
  ws_srv_->SendMessage(response.dump());
}

void ClientGame::h_BuildPotential(nlohmann::json& msg) {
  spdlog::get(LOGGER)->debug("ClientGame::m_BuildPotential: {}", msg.dump());
  // If first call: send message to server, checking wether neuron can be build.
  if (msg.size() == 0) {
    spdlog::get(LOGGER)->debug("ClientGame::m_BuildPotential: 1");
    char cmd = contexts_.at(current_context_).cmd();
    int num = (history_.size() > 1 && std::isdigit(history_[history_.size()-2])) ? history_[history_.size()-2] - '0' : 1;
    int unit = (cmd == 'e') ? EPSP : (cmd == 'i') ? IPSP : MACRO;
    nlohmann::json response = {{"command", "check_build_potential"}, {"username", username_}, {"data",
      {{"unit", unit}, {"num", num}} }};
    ws_srv_->SendMessage(response.dump());
  }
  else if (msg["data"].contains("start_pos")) {
    spdlog::get(LOGGER)->debug("ClientGame::m_BuildPotential: 2");
    RemovePickContext(CONTEXT_RESOURCES);
    nlohmann::json response = {{"command", "check_build_potential"}, {"username", username_}, {"data",
      {{"unit", msg["data"]["unit"]}, {"num", msg["data"]["num"]}, {"start_pos", msg["data"]["start_pos"]}} }};
    ws_srv_->SendMessage(response.dump());
  }
  else if (msg["data"].contains("positions")) {
    spdlog::get(LOGGER)->debug("ClientGame::m_BuildPotential: 3");
    SwitchToPickContext(msg["data"]["positions"], "Select synapse", "build_potential", msg);
  }
  msg = nlohmann::json();
}
void ClientGame::h_ToResourceContext(nlohmann::json&) {
  SwitchToResourceContext("Aborted adding neuron/ potential");
}

void ClientGame::h_BuildNeuron(nlohmann::json& msg) {
  char cmd = contexts_.at(current_context_).cmd();
  spdlog::get(LOGGER)->debug("ClientGame::m_BuildNeuron: {}", msg.dump());
  // If first call: send message to server, checking wether neuron can be build.
  if (msg.size() == 0) {
    int unit = (cmd == 'A') ? ACTIVATEDNEURON : (cmd == 'S') ? SYNAPSE : NUCLEUS;
    nlohmann::json response = {{"command", "check_build_neuron"}, {"username", username_}, {"data",
      {{"unit", unit}} }};
    ws_srv_->SendMessage(response.dump());
  }
  // Final call with position: send message to server to build neuron.
  else if (msg["data"].contains("pos")) {
    nlohmann::json response = {{"command", "build_neuron"}, {"username", username_}, {"data",
      {{"unit", msg["data"]["unit"]}, {"pos", utils::PositionFromVector(msg["data"]["pos"])}} }};
    ws_srv_->SendMessage(response.dump());
  }
  // Second call no start-position: Add Pick-Context to select start position
  else if (!msg["data"].contains("start_pos")) {
    if (msg["data"]["unit"] == NUCLEUS)
      SwitchToPickContext(msg["data"]["positions"], "Select start-position", "build_neuron", msg);
    else
      SwitchToPickContext(msg["data"]["positions"], "Select nucleus", "build_neuron", msg);
  }
  // Second call with start-position, or third call with start position from marker: select pos from field
  else if (msg["data"].contains("start_pos")) {
    RemovePickContext();
    int range = (msg["data"]["unit"] == NUCLEUS) ? 999 : msg["data"]["range"].get<int>();
    SwitchToFieldContext(msg["data"]["start_pos"], range, "build_neuron", msg,
      "Select position to build " + units_tech_name_mapping.at(msg["data"]["unit"]));
  }
  msg = nlohmann::json();
}

void ClientGame::h_SendSelectSynapse(nlohmann::json& msg) {
  spdlog::get(LOGGER)->debug("ClientGame::h_SendSelectSynapse: {}", msg.dump());
  // If first call: initalize synape-context
  if (msg.size() == 0) {
    spdlog::get(LOGGER)->debug("ClientGame::h_SendSelectSynapse: sending initial request...");
    SwitchToSynapseContext();
    GetPosition req(username_, "select_synapse", {{PLAYER, GetPositionInfo(SYNAPSE)}});
    ws_srv_->SendMessage(req.json().dump());
  }
  // If no start-pos selected, select start-pos (synapse)
  else if (!msg["data"].contains("start_pos")) {
    spdlog::get(LOGGER)->debug("ClientGame::h_SendSelectSynapse: setting up pick context...");
    std::vector<position_t> positions = msg["data"]["positions"][0];
    if (positions.size() == 0) {
      spdlog::get(LOGGER)->debug("ClientGame::h_SendSelectSynapse: no synapse!");
      drawrer_.set_msg("No synapse.");
      SwitchToResourceContext();
    }
    else if (positions.size() == 1) {
      spdlog::get(LOGGER)->debug("ClientGame::h_SendSelectSynapse: only one synapse...");
      SwitchToSynapseContext(nlohmann::json({{"data", {{"synapse_pos", positions.front()}} }}));
      drawrer_.set_msg("Choose what to do.");
      drawrer_.AddMarker(SYNAPSE_MARKER, positions.front(), COLOR_MARKED);
    }
    else {
      spdlog::get(LOGGER)->debug("ClientGame::h_SendSelectSynapse: switching to pick context...");
      SwitchToPickContext(positions, "Select synapse", "select_synapse", msg);
    }
  }
  // If start-pos selected, go back to synapse-context, and set initial data.
  else if (msg["data"].contains("start_pos")) {
    spdlog::get(LOGGER)->debug("ClientGame::h_SendSelectSynapse: back to synapse-context...");
    RemovePickContext();
    position_t synapse_pos = utils::PositionFromVector(msg["data"]["start_pos"]);
    SwitchToSynapseContext(nlohmann::json({{"data", {{"synapse_pos", synapse_pos}} }}));
    drawrer_.set_msg("Choose what to do.");
    drawrer_.AddMarker(SYNAPSE_MARKER, synapse_pos, COLOR_MARKED);
  }
  msg = nlohmann::json();
}

void ClientGame::h_SetWPs(nlohmann::json& msg) {
  spdlog::get(LOGGER)->debug("Calling set-wp with data: {}", msg.dump());
  // Final call (set new way point)
  if (msg["data"].contains("pos")) {
    spdlog::get(LOGGER)->debug("h_SetWPs: Setting new wp! context: {}", current_context_);
    // Send request to set way-point.
    nlohmann::json response = {{"command", "set_way_point"}, {"username", username_}, {"data",
      {{"pos", msg["data"]["pos"]}, {"synapse_pos", msg["data"]["synapse_pos"]}, {"num", msg["data"]["num"]}} }};
    ws_srv_->SendMessage(response.dump());
    spdlog::get(LOGGER)->debug("h_SetWPs: done. Current context: {}", current_context_);
  }
  // Thirst call (select field position)
  else if (msg["data"].contains("start_pos")) {
    RemovePickContext(CONTEXT_SYNAPSE);
    SwitchToFieldContext(msg["data"]["start_pos"], 1000, "set_wps", msg, "Select new way-point position.", {'q'});
  }
  // Second call (select start position)
  else if (msg["data"].contains("positions")) {
    // Set synapse-position.
    msg["data"]["synapse_pos"] = contexts_.at(current_context_).data()["data"]["synapse_pos"];
    msg["data"]["num"] = contexts_.at(current_context_).data()["data"]["num"];
    // Print ways
    std::vector<position_t> way = msg["data"]["positions"][1];
    for (const auto& it : way)
      drawrer_.AddMarker(WAY_MARKER, it, COLOR_MARKED);
    std::vector<position_t> way_points = msg["data"]["positions"][2];
    for (unsigned int i=0; i<way_points.size(); i++)
      drawrer_.AddMarker(WAY_MARKER, way_points[i], COLOR_MARKED, utils::CharToString('1', i));
    // Set up pick-context
    std::vector<position_t> center_positions = msg["data"]["positions"][0];
    SwitchToPickContext(center_positions, "select start position", "set_wps", msg, {'q'});
  }
  // First call (request positions)
  else {
    // If "msg" is contained, print message
    if (msg["data"].contains("msg"))
      drawrer_.set_msg(msg["data"]["msg"]);
    // Set "1" as number (indicating first way-point) if num is not given.
    if (!msg["data"].contains("num"))
      msg["data"]["num"] = 1;
    // If num is -1, this indicates, that no more way-point can be set: switch
    // to resource-context or stay at synapse-context depending on settings.
    if (msg["data"]["num"] == -1)
      FinalSynapseContextAction(msg["data"]["synapse_pos"]);
    // Otherwise, request inital data.
    else {
      // Memorize number.
      nlohmann::json data = contexts_.at(current_context_).data();
      data["data"]["num"] = msg["data"]["num"];
      contexts_.at(current_context_).set_data(data);
      // Request inital data.
      position_t synapse_pos = utils::PositionFromVector(msg["data"]["synapse_pos"]);
      GetPosition req(username_, "set_wps", {{CURRENT_WAY, GetPositionInfo(synapse_pos)}, 
          {CURRENT_WAY_POINTS, GetPositionInfo(synapse_pos)}, {CENTER, GetPositionInfo()}});
      ws_srv_->SendMessage(req.json().dump());
    }
  }
  msg = nlohmann::json();
}

void ClientGame::h_EpspTarget(nlohmann::json& msg) {
  spdlog::get(LOGGER)->debug("Calling epsp-target with data: {}", msg.dump());
  // final call (set epsp target and reset to synapse-context)
  if (msg["data"].contains("pos")) {
    // Reset synapse-context or go back to resource (standard)-context.
    FinalSynapseContextAction(msg["data"]["synapse_pos"]);
    // Send request to server.
    nlohmann::json response = {{"command", "set_epsp_target"}, {"username", username_}, {"data",
      {{"pos", msg["data"]["pos"]}, {"synapse_pos", msg["data"]["synapse_pos"]}} }};
    ws_srv_->SendMessage(response.dump());
  }
  // third call (select field position)
  else if (msg["data"].contains("start_pos")) {
    RemovePickContext();
    SwitchToFieldContext(msg["data"]["start_pos"], 1000, "epsp_target", msg, "Select epsp-target position.", {'q'});
  }
  // Second call (select start position)
  else if (msg["data"].contains("positions")) {
    // Set synapse-position.
    msg["data"]["synapse_pos"] = contexts_.at(current_context_).data()["data"]["synapse_pos"];
    std::vector<position_t> enemy_nucleus_positions = msg["data"]["positions"][0];
    // If only one enemy nucleus, set-up field-select and add start-pos to message-json.
    if (enemy_nucleus_positions.size() == 1) {
      msg["data"]["start_pos"] = enemy_nucleus_positions.front();
      SwitchToFieldContext(enemy_nucleus_positions.front(), 1000, "epsp_target", msg, "Select epsp-target position.", 
          {'q'});
    }
    // Create pick-context to select synapse (add quit-handler to return to normal context).
    else {
      SwitchToPickContext(msg["data"]["positions"][0], "Select select enemy base", "epsp_target", msg, {'q'});
    }
    // Display current target on field.
    std::vector<std::vector<int>> target_positions = msg["data"]["positions"][1];
    if (target_positions.size() > 0)
      drawrer_.AddMarker(TARGETS_MARKER, utils::PositionFromVector(target_positions.front()), COLOR_MARKED, "T");
  }
  // First call (request positions)
  else {
    GetPosition req(username_, "epsp_target", {{ENEMY, GetPositionInfo(NUCLEUS)}, 
        {TARGETS, GetPositionInfo(EPSP, msg["data"]["synapse_pos"].get<position_t>())}});
    ws_srv_->SendMessage(req.json().dump());
  }
  msg = nlohmann::json();
}

void ClientGame::h_IpspTarget(nlohmann::json& msg) {
  spdlog::get(LOGGER)->debug("Calling ipsp-target with data: {}", msg.dump());
  // final call (set ipsp target and reset to synapse-context)
  if (msg["data"].contains("pos")) {
    // Reset synapse-context or go back to resource (standard)-context.
    FinalSynapseContextAction(msg["data"]["synapse_pos"]);
    // Send request to server.
    nlohmann::json response = {{"command", "set_ipsp_target"}, {"username", username_}, {"data",
      {{"pos", msg["data"]["pos"]}, {"synapse_pos", msg["data"]["synapse_pos"]}} }};
    ws_srv_->SendMessage(response.dump());
  }
  // third call (select field position)
  else if (msg["data"].contains("start_pos")) {
    RemovePickContext(CONTEXT_SYNAPSE);
    SwitchToFieldContext(msg["data"]["start_pos"], 1000, "ipsp_target", msg, "Select ipsp-target position.", {'q'});
  }
  // Second call (select start position)
  else if (msg["data"].contains("positions")) {
    // Set synapse-position.
    msg["data"]["synapse_pos"] = contexts_.at(current_context_).data()["data"]["synapse_pos"];
    std::vector<position_t> enemy_nucleus_positions = msg["data"]["positions"][0];
    // If only one enemy nucleus, set-up field-select and add start-pos to message-json.
    if (enemy_nucleus_positions.size() == 1) {
      msg["data"]["start_pos"] = enemy_nucleus_positions.front();
      SwitchToFieldContext(enemy_nucleus_positions.front(), 1000, "ipsp_target", msg, "Select ipsp-target position.", 
          {'q'});
    }
    // Create pick-context to select synapse (add quit-handler to return to normal context).
    else {
      SwitchToPickContext(msg["data"]["positions"][0], "Select select enemy base", "ipsp_target", msg, {'q'});
    }
    // Display current target on field.
    std::vector<std::vector<int>> target_positions = msg["data"]["positions"][1];
    if (target_positions.size() > 0)
      drawrer_.AddMarker(TARGETS_MARKER, utils::PositionFromVector(target_positions.front()), COLOR_MARKED, "T");
  }
  // First call (request positions)
  else {
    GetPosition req(username_, "ipsp_target", {{ENEMY, GetPositionInfo(NUCLEUS)}, 
        {TARGETS, GetPositionInfo(IPSP, msg["data"]["synapse_pos"].get<position_t>())}});
    ws_srv_->SendMessage(req.json().dump());
  }
  msg = nlohmann::json();
}

void ClientGame::h_SwarmAttack(nlohmann::json& msg) {
  // Depending on setting: switch to resource context
  FinalSynapseContextAction(msg["data"]["synapse_pos"]);
  // Send request to server toggling swarm-attack.
  nlohmann::json response = {{"command", "toggle_swarm_attack"}, {"username", username_}, {"data",
    {{"pos", msg["data"]["synapse_pos"] }} }};
  ws_srv_->SendMessage(response.dump());
}

void ClientGame::h_ResetOrQuitSynapseContext(nlohmann::json& msg) {
  spdlog::get(LOGGER)->debug("ClientGame::h_ResetOrQuitSynapseContext: {}", msg.dump());
  nlohmann::json data = contexts_.at(current_context_).data()["data"];
  if (data.size() > 1) {
    position_t synapse_pos = utils::PositionFromVector(data.value("synapse_pos", data["start_pos"]));
    SwitchToSynapseContext(nlohmann::json({{"data", {{"synapse_pos", synapse_pos}} }}));
    drawrer_.set_viewpoint(CONTEXT_RESOURCES);
    drawrer_.ClearMarkers();
    drawrer_.AddMarker(SYNAPSE_MARKER, synapse_pos, COLOR_MARKED);
  }
  else {
    drawrer_.ClearMarkers();
    SwitchToResourceContext();
  }
}

void ClientGame::h_AddPosition(nlohmann::json&) {
  spdlog::get(LOGGER)->debug("ClientGame::AddPosition: action: {}", contexts_.at(current_context_).action());
  position_t pos = drawrer_.field_pos();
  std::string action = contexts_.at(current_context_).action();
  if (!drawrer_.InGraph(pos)) {
    drawrer_.set_msg("Position not reachable!");
    return;
  }
  if (action == "build_neuron" && !drawrer_.Free(pos)) {
    drawrer_.set_msg("Position not free!");
    return;
  }

  nlohmann::json msg = contexts_.at(current_context_).data();
  msg["data"]["pos"] = pos;
  if (action == "build_neuron")
      h_BuildNeuron(msg);
  else if (action == "set_wps") 
    h_SetWPs(msg);
  else if (action == "ipsp_target") 
    h_IpspTarget(msg);
  else if (action == "epsp_target") 
    h_EpspTarget(msg);
}

void ClientGame::h_AddStartPosition(nlohmann::json&) {
  spdlog::get(LOGGER)->debug("ClientGame::AddStartPosition: action: {}", contexts_.at(current_context_).action());
  nlohmann::json msg = contexts_.at(current_context_).data();
  msg["data"]["start_pos"] = drawrer_.GetMarkerPos(PICK_MARKER, utils::CharToString(history_.back(), 0));
  std::string action = contexts_.at(current_context_).action();
  if (action == "build_potential")
    h_BuildPotential(msg);
  else if (action == "build_neuron")
    h_BuildNeuron(msg);
  else if (action == "select_synapse") 
    h_SendSelectSynapse(msg);
  else if (action == "set_wps") 
    h_SetWPs(msg);
  else if (action == "ipsp_target") 
    h_IpspTarget(msg);
  else if (action == "epsp_target") 
    h_EpspTarget(msg);
}

void ClientGame::SwitchToResourceContext(std::string msg) {
  spdlog::get(LOGGER)->debug("ClientGame::SwitchToResourceContext");
  std::shared_lock sl(mutex_context_);
  current_context_ = CONTEXT_RESOURCES;
  drawrer_.set_topline(contexts_.at(current_context_).topline());
  drawrer_.set_viewpoint(current_context_);
  if (msg != "")
    drawrer_.set_msg(msg);
}

void ClientGame::SwitchToSynapseContext(nlohmann::json msg) {
  spdlog::get(LOGGER)->debug("ClientGame::SwitchToSynapseContext");
  std::shared_lock sl(mutex_context_);
  current_context_ = CONTEXT_SYNAPSE;
  contexts_.at(current_context_).set_data(msg);
  drawrer_.set_topline(contexts_.at(current_context_).topline());
}

void ClientGame::FinalSynapseContextAction(position_t synapse_pos) {
  spdlog::get(LOGGER)->debug("ClientGame::FinalSynapseContextAction");
  // Switch (back) to synapse-context (clear only target-markers, leave synapse marked)
  if (stay_in_synapse_menu_) {
    SwitchToSynapseContext(nlohmann::json({{"data", {{"synapse_pos", synapse_pos}} }}));
    drawrer_.ClearMarkers(TARGETS_MARKER);
    drawrer_.ClearMarkers(WAY_MARKER);
  }
  // End synapse-context and go to resource-context (clear ALL markers).
  else {
    SwitchToResourceContext();
    drawrer_.ClearMarkers();
  }
  drawrer_.set_viewpoint(CONTEXT_RESOURCES);
}

void ClientGame::SwitchToPickContext(std::vector<position_t> positions, std::string msg, std::string action, 
    nlohmann::json data, std::vector<char> slip_handlers) {
  spdlog::get(LOGGER)->info("ClientGame::CreatePickContext: switched to pick context.");
  std::shared_lock sl(mutex_context_);
  // Get all handlers and add markers to drawrer.
  drawrer_.ClearMarkers(PICK_MARKER);
  std::map<char, void(ClientGame::*)(nlohmann::json&)> new_handlers = {{'t', &ClientGame::h_ChangeViewPoint}};
  for (unsigned int i=0; i<positions.size(); i++) {
    new_handlers['a'+i] = &ClientGame::h_AddStartPosition;
    drawrer_.AddMarker(PICK_MARKER, positions[i], COLOR_AVAILIBLE, utils::CharToString('a', i));
  }
  // Create pick context
  Context context = Context(msg, new_handlers, {{" [a, b, ...] to pick from", 
      COLOR_DEFAULT}, {" [h]elp ", COLOR_DEFAULT}, {" [q]uit ", COLOR_DEFAULT}});
  context.set_action(action);
  context.set_data(data);
  // Let handlers slip through.
  for (const auto& it : slip_handlers) {
    if (contexts_.at(current_context_).eventmanager().handlers().count(it) > 0)
      context.eventmanager().AddHandler(it, contexts_.at(current_context_).eventmanager().handlers().at(it));
  }
  current_context_ = CONTEXT_PICK;
  contexts_[current_context_] = context;
  drawrer_.set_topline(contexts_.at(current_context_).topline());
}

void ClientGame::SwitchToFieldContext(position_t pos, int range, std::string action, nlohmann::json data,
    std::string msg, std::vector<char> slip_handlers) {
  spdlog::get(LOGGER)->debug("ClientGame::SwitchToFieldContext: switching...");
  std::shared_lock sl(mutex_context_);
  // Set data for field-context and empy old positions.
  contexts_.at(CONTEXT_FIELD).set_action(action);
  contexts_.at(CONTEXT_FIELD).set_data(data);
  // Clear potential sliped handlers
  std::vector<char> handlers_to_remove;
  for (const auto& it : contexts_.at(CONTEXT_FIELD).eventmanager().handlers()) {
    if (handlers_[CONTEXT_FIELD].count(it.first) == 0)
      handlers_to_remove.push_back(it.first);
  }
  for (const auto& it : slip_handlers) 
    contexts_.at(CONTEXT_FIELD).eventmanager().handlers().erase(it);
  // Let handlers slip through.
  for (const auto& it : slip_handlers) {
    if (contexts_.at(current_context_).eventmanager().handlers().count(it) > 0)
      contexts_.at(CONTEXT_FIELD).eventmanager().AddHandler(it, 
          contexts_.at(current_context_).eventmanager().handlers().at(it));
  }
  current_context_ = CONTEXT_FIELD;

  // Set up drawrer/
  spdlog::get(LOGGER)->debug("Drawrer setting for field context.");
  drawrer_.set_field_start_pos(pos);
  drawrer_.set_range({pos, range});
  drawrer_.set_viewpoint(current_context_);
  drawrer_.set_topline(contexts_.at(current_context_).topline());
  drawrer_.set_msg(msg);
  spdlog::get(LOGGER)->debug("ClientGame::SwitchToFieldContext: done.");
}

void ClientGame::RemovePickContext(int new_context) {
  spdlog::get(LOGGER)->debug("Removing pick context...");
  std::shared_lock sl(mutex_context_);
  if (contexts_.count(CONTEXT_PICK) > 0)
    contexts_.erase(CONTEXT_PICK);
  drawrer_.ClearMarkers();
  if (new_context != -1) {
    current_context_ = new_context;
    drawrer_.set_viewpoint(current_context_);
    drawrer_.set_topline(contexts_.at(current_context_).topline());
  }
}

void ClientGame::m_SelectMode(nlohmann::json& msg) {
  spdlog::get(LOGGER)->debug("ClientGame::m_SelectMode: {}", msg.dump());
  // Print welcome text.
  if (show_full_welcome_text_)
    drawrer_.PrintCenteredParagraphs(texts::welcome);
  else 
    drawrer_.PrintCenteredParagraphs(texts::welcome_reduced);
  
  // Update msg
  msg["command"] = "init_game";
  msg["data"] = {{"lines", drawrer_.field_height()}, {"cols", drawrer_.field_width()}, {"base_path", base_path_}};
 
  // Select single-player, mulit-player (host/ client), observer.
  choice_mapping_t mapping = {
    {SINGLE_PLAYER, {"singe-player", (!muliplayer_availible_) ? COLOR_AVAILIBLE : COLOR_DEFAULT}}, 
    {MULTI_PLAYER, {"muli-player (host)", (muliplayer_availible_) ? COLOR_AVAILIBLE : COLOR_DEFAULT}}, 
    {MULTI_PLAYER_CLIENT, {"muli-player (client)", (muliplayer_availible_) ? COLOR_AVAILIBLE : COLOR_DEFAULT}}, 
    {TUTORIAL, {"tutorial", COLOR_AVAILIBLE}},
    {OBSERVER, {"watch ki", COLOR_AVAILIBLE}},
    {SETTINGS, {"settings", COLOR_AVAILIBLE}}
  };
  mode_ = SelectInteger("Select mode", true, mapping, {mapping.size()+1}, "Mode not available");
  if (mode_ == SETTINGS) {
    EditSettings();
    m_SelectMode(msg);
  }
  drawrer_.set_mode(mode_);
  msg["data"]["mode"] = mode_;
  // Tutorial: show first tutorial message, then change mode (in request) to normal singe-player.
  if (mode_ == TUTORIAL) {
    drawrer_.PrintCenteredParagraphs(texts::tutorial_start);
  }
  // If host, then also select number of players.
  else if (mode_ == MULTI_PLAYER) {
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
  msg["data"] = nlohmann::json::object();

  // Load map-audio.
  audio_file_path_ = SelectAudio("select map");
  // If not on same-device send audio file.
  if (!ws_srv_->same_device()) {
    std::string content = "";
    while (true) {
      // Load file and create content.
      std::filesystem::path p = audio_file_path_;
      content = p.filename().string() + "$" + utils::GetMedia(audio_file_path_);
      spdlog::get(LOGGER)->info("File size: {}", content.size());
      // Check file size.
      if (content.size() < pow(2, 25))
        break;
      // If insufficient, print error and select audio again
      drawrer_.ClearField();
      drawrer_.PrintCenteredLine(LINES/2, "File size too big! (press any key to continue.)");
      getch();
      drawrer_.ClearField();
      audio_file_path_ = SelectAudio("select map");
    }
    ws_srv_->SendMessageBinary(content);
  }
  // Otherwise send only path.
  else 
    msg["data"]["map_path"] = audio_file_path_;
  // Add map-file name.
  std::filesystem::path p = audio_file_path_;
  msg["data"]["map_name"] = p.filename().string();

  // If observer load audio for two ai-files (no need to send audi-file: only analysed data)
  if (mode_ == OBSERVER) {
    msg["data"]["ais"] = nlohmann::json::object();
    msg["data"]["base_path"] = base_path_;
    for (unsigned int i = 0; i<2; i++) {
      std::string source_path = SelectAudio("select ai sound " + std::to_string(i+1));
      Audio audio(base_path_);
      audio.set_source_path(source_path);
      audio.Analyze();
      msg["data"]["ais"][source_path] = audio.GetAnalyzedData();
    }
  }
}

void ClientGame::m_PrintMsg(nlohmann::json& msg) {
  drawrer_.ClearField();
  drawrer_.PrintCenteredLine(LINES/2, msg["data"]["msg"]);
  refresh();
  msg = nlohmann::json();
}

void ClientGame::m_InitGame(nlohmann::json& msg) {
  clear();
  refresh();
  drawrer_.set_transfer(msg["data"]);
  drawrer_.PrintGame(false, false, current_context_);
  status_ = RUNNING;
  current_context_ = CONTEXT_RESOURCES;
  drawrer_.set_msg(contexts_.at(current_context_).msg());
  drawrer_.set_topline(contexts_.at(current_context_).topline());
  
  audio_.set_source_path(audio_file_path_);
  if (music_on_)
    audio_.play();
  msg = nlohmann::json();
}

void ClientGame::m_UpdateGame(nlohmann::json& msg) {
  drawrer_.UpdateTranser(msg["data"]);
  drawrer_.PrintGame(false, false, current_context_);
  msg = nlohmann::json();
}

void ClientGame::m_UpdateLobby(nlohmann::json& msg) {
  // Set
  if (current_context_ != VP_LOBBY) {
    status_ = RUNNING;
    current_context_ = CONTEXT_LOBBY;
    drawrer_.set_viewpoint(current_context_);
  }
  // Update Lobby
  drawrer_.UpdateLobby(msg["data"]);
  drawrer_.PrintLobby();
  msg = nlohmann::json();
}

void ClientGame::m_SetMsg(nlohmann::json& msg) {
  drawrer_.set_msg(msg["data"]["msg"]);
  msg = nlohmann::json();
}

void ClientGame::m_GameEnd(nlohmann::json& msg) {
  drawrer_.set_stop_render(true);
  drawrer_.ClearField();
  drawrer_.PrintCenteredLine(LINES/2, msg["data"]["msg"].get<std::string>());
  drawrer_.PrintCenteredLine(LINES/2+2, " [Press 'q' to leave game and 'h'/'l' to cycle statistics]");
  drawrer_.set_statistics(msg["data"]["statistics"]);
  refresh();
  ws_srv_->Stop();
  std::unique_lock ul(mutex_context_);
  current_context_ = CONTEXT_POST_GAME;
  drawrer_.set_viewpoint(VP_POST_GAME);
  msg = nlohmann::json();
}

void ClientGame::m_SetUnit(nlohmann::json& msg) {
  drawrer_.AddNewUnitToPos(msg["data"]["pos"], msg["data"]["unit"], msg["data"]["color"]);
  SwitchToResourceContext("Success!");
  drawrer_.PrintGame(false, false, current_context_);
  msg = nlohmann::json();
}

void ClientGame::m_SetUnits(nlohmann::json& msg) {
  spdlog::get(LOGGER)->info("ClientGame::m_SetUnits");
  std::map<position_t, int> neurons = msg["data"]["neurons"];
  int color = msg["data"]["color"];
  for (const auto& it : neurons) 
    drawrer_.AddNewUnitToPos(it.first, it.second, color);
  spdlog::get(LOGGER)->debug("ClientGame::m_SetUnits. Done. Printing field...");
  drawrer_.PrintGame(false, false, current_context_);
  spdlog::get(LOGGER)->debug("ClientGame::m_SetUnits. Done.");
  msg = nlohmann::json();
}

// tutorial 

void ClientGame::h_TutorialGetOxygen(nlohmann::json& msg) {
  spdlog::get(LOGGER)->info("h_TutorialGetOxygen");
  Pause();
  contexts_.at(CONTEXT_TEXT).init_text(texts::tutorial_get_oxygen, current_context_);
  current_context_ = CONTEXT_TEXT;
  h_TextPrint();
  eventmanager_tutorial_.RemoveHandler("init_game");
}

void ClientGame::h_TutorialSetUnit(nlohmann::json& original_message) {
  nlohmann::json data = original_message["data"];
  std::vector<texts::paragraphs_field_t> texts;
  // Get text ad do potential other actions
  if (data["unit"] == RESOURCENEURON && data["resource"] == OXYGEN && !tutorial_.oxygen_) {
    texts.push_back(texts::tutorial_get_glutamat);
    tutorial_.oxygen_ = true;
  }
  else if (data["unit"] == RESOURCENEURON && data["resource"] == GLUTAMATE && !tutorial_.glutamate_) {
    texts.push_back(texts::tutorial_build_activated_neurons);
    tutorial_.glutamate_ = true;
  }
  else if (data["unit"] == RESOURCENEURON && data["resource"] == POTASSIUM && !tutorial_.potassium_) {
    texts.push_back(texts::tutorial_build_synapse);
    tutorial_.potassium_ = true;
  }
  else if (data["unit"] == RESOURCENEURON && data["resource"] == CHLORIDE && !tutorial_.chloride_) {
    texts.push_back(texts::tutorial_build_ipsp);
    tutorial_.chloride_ = true;
  }
  else if (data["unit"] == RESOURCENEURON && data["resource"] == DOPAMINE && !tutorial_.dopamine_) {
    texts.push_back(texts::tutorial_technologies_dopamine);
    tutorial_.dopamine_ = true;
    if (tutorial_.serotonin_)
      texts.push_back(texts::tutorial_technologies_all);
    // Always add how-to
    texts.push_back(texts::tutorial_technologies_how_to);
  }
  else if (data["unit"] == RESOURCENEURON && data["resource"] == SEROTONIN && !tutorial_.serotonin_) {
    texts.push_back(texts::tutorial_technologies_seretonin);
    tutorial_.serotonin_ = true;
    if (tutorial_.dopamine_) {
      texts.push_back(texts::tutorial_technologies_all);
      texts.push_back(texts::tutorial_technologies_how_to);
    }
  }
  else if (data["unit"] == ACTIVATEDNEURON && tutorial_.activated_neurons_ == 3) {
    texts.push_back(texts::tutorial_get_potassium);
  }
  else if (data["unit"] == ACTIVATEDNEURON && tutorial_.activated_neurons_ == 4) {
    texts.push_back(texts::tutorial_bound_resources);
  }
  else if (data["unit"] == SYNAPSE && tutorial_.synapses_ == 0) {
    texts.push_back(texts::tutorial_build_potential);
    tutorial_.synapses_++;
  }
  // If text was set, print text:
  if (texts.size() > 0) {
    Pause();
    // Add all texts.
    auto final_text = texts[0];
    for (unsigned int i=1; i<texts.size(); i++) {
      final_text.insert(final_text.end(), std::make_move_iterator(texts[i].begin()), 
        std::make_move_iterator(texts[i].end()));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(800));
    contexts_.at(CONTEXT_TEXT).init_text(final_text, current_context_);
    current_context_ = CONTEXT_TEXT;
    h_TextPrint();
  }
}

void ClientGame::h_TutorialScouted(nlohmann::json& original_message) {
  texts::paragraphs_field_t text;
  // On first discoring enemy terretoris: tutorial for selecting targets.
  if (!tutorial_.discovered_) {
    text = texts::tutorial_select_target;
    tutorial_.discovered_ = true;
  }
  // When more than 2 enemy activated_neurons are discovered: tutorial for final, large attack.
  else {
    // Check number 
    int counter = 0;
    for (const auto& it : original_message["data"]["neurons"].get<std::map<position_t, int>>()) 
      if (it.second == ACTIVATEDNEURON) counter++;
    if (counter >= 2 && tutorial_.chloride_ && tutorial_.dopamine_)  {
      text = texts::tutorial_final_attack;
      tutorial_.action_active_ = true;
      tutorial_.action_ = 0;
      eventmanager_tutorial_.RemoveHandler("set_units");
    }
  }
  // If text was set, print text:
  if (text.size() > 0) {
    Pause();
    std::this_thread::sleep_for(std::chrono::milliseconds(800));
    contexts_.at(CONTEXT_TEXT).init_text(text, current_context_);
    current_context_ = CONTEXT_TEXT;
    h_TextPrint();
  }
}

void ClientGame::h_TutorialBuildNeuron(nlohmann::json& original_message) {
  texts::paragraphs_field_t text;
  // Get text ad do potential other actions
  if (original_message["data"]["unit"] == ACTIVATEDNEURON && tutorial_.activated_neurons_ == 0) {
    text = texts::tutorial_first_build;
    tutorial_.activated_neurons_++;
  }
  else if (original_message["data"]["unit"] == ACTIVATEDNEURON)
    tutorial_.activated_neurons_++;
 
  // If text was set, print text:
  if (text.size() > 0) {
    Pause();
    std::this_thread::sleep_for(std::chrono::milliseconds(800));
    contexts_.at(CONTEXT_TEXT).init_text(text, current_context_);
    current_context_ = CONTEXT_TEXT;
    h_TextPrint();
  }
}

void ClientGame::h_TutorialAction(nlohmann::json&) {
  texts::paragraphs_field_t text;
  // set ways
  if (tutorial_.action_ == 0) {
    text = texts::tutorial_final_attack_set_way;
  }
  // set targets
  else if (tutorial_.action_ == 1) {
    text = texts::tutorial_final_attack_set_targets;
  }
  // launch attack
  else if (tutorial_.action_ == 2) {
    text = texts::tutorial_final_attack_launch_attack;
    // reset action
    tutorial_.action_active_ = false;
    tutorial_.action_ = 0;
  }

  // If text was set, print text:
  if (text.size() > 0) {
    tutorial_.action_++;
    Pause();
    std::this_thread::sleep_for(std::chrono::milliseconds(800));
    contexts_.at(CONTEXT_TEXT).init_text(text, current_context_);
    current_context_ = CONTEXT_TEXT;
    h_TextPrint();
  }
}

void ClientGame::h_TutorialSetMessage(nlohmann::json& original_message) {
  texts::paragraphs_field_t text;
  // Get text ad do potential other actions
  if (original_message["data"]["msg"] == "Epsp target for this synapse set" && !tutorial_.epsp_target_set_) {
    text = texts::tutorial_strong_attack;
    tutorial_.epsp_target_set_ = true;
  }
  
  // If text was set, print text:
  if (text.size() > 0) {
    Pause();
    std::this_thread::sleep_for(std::chrono::milliseconds(800));
    contexts_.at(CONTEXT_TEXT).init_text(text, current_context_);
    current_context_ = CONTEXT_TEXT;
    h_TextPrint();
  }
}

void ClientGame::h_TutorialUpdateGame(nlohmann::json& original_message) {
  texts::paragraphs_field_t text;
  Transfer t(original_message["data"]);
  
  // first enemy attack-launch
  if (t.potentials().size() > 0 && tutorial_.first_attack_) {
    text = texts::tutorial_first_attack;
    tutorial_.first_attack_ = false;
  }

  if (t.players().at(username_).first.front() != '0' && tutorial_.first_damage_) {
    text = texts::tutorial_first_damage;
    tutorial_.first_damage_ = false;
  }
 
  // If text was set, print text:
  if (text.size() > 0) {
    Pause();
    std::this_thread::sleep_for(std::chrono::milliseconds(800));
    contexts_.at(CONTEXT_TEXT).init_text(text, current_context_);
    current_context_ = CONTEXT_TEXT;
    h_TextPrint();
  }
}

void ClientGame::Pause() {
  pause_ = true;
  drawrer_.set_stop_render(true);
  nlohmann::json msg = {{"command", "set_pause_on"}, {"username", username_}, {"data", nlohmann::json()}};
  ws_srv_->SendMessage(msg.dump());
  audio_.Pause();
}

void ClientGame::UnPause() {
  pause_ = false;
  nlohmann::json msg = {{"command", "set_pause_off"}, {"username", username_}, {"data", nlohmann::json()}};
  ws_srv_->SendMessage(msg.dump());
  audio_.Unpause();
  drawrer_.set_stop_render(false);
}

// text
void ClientGame::h_TextNext(nlohmann::json& msg) {
  // Check if last text was reached.
  if (contexts_.at(current_context_).last_text())
    h_TextQuit();
  // Otherwise increase num and then print text.
  else {
    unsigned int num = contexts_.at(CONTEXT_TEXT).num();
    contexts_.at(CONTEXT_TEXT).set_num(num+1);
    h_TextPrint();
  }
}

void ClientGame::h_TextPrev(nlohmann::json&) {
  unsigned int num = contexts_.at(CONTEXT_TEXT).num();
  if (num == 0) 
    return;
  contexts_.at(CONTEXT_TEXT).set_num(num-1);
  h_TextPrint();
}

void ClientGame::h_TextQuit(nlohmann::json&) {
  h_TextQuit();
}
void ClientGame::h_TextQuit() {
  std::shared_lock sl(mutex_context_);
  current_context_ = contexts_.at(CONTEXT_TEXT).last_context();
  drawrer_.set_topline(contexts_.at(current_context_).topline());
  drawrer_.set_viewpoint(current_context_);
  UnPause();
}

void ClientGame::h_TextPrint() {
  drawrer_.ClearField();
  auto paragraph = contexts_.at(current_context_).get_paragraph();
  drawrer_.PrintCenteredParagraphAndMiniFields(paragraph.first, paragraph.second, true);
  refresh();
}

// Selection methods

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
    else if (mapping.count(int_choice) > 0 && (mapping.at(int_choice).second == COLOR_AVAILIBLE || !omit))
      return int_choice;
    else if (mapping.count(int_choice) > 0 && mapping.at(int_choice).second != COLOR_AVAILIBLE && omit)
      drawrer_.PrintCenteredLine(LINES/2+counter+5, "Selection not available (" + error_msg + "): " 
          + std::to_string(int_choice));
    else 
      drawrer_.PrintCenteredLine(LINES/2+counter+5, "Wrong selection: " + std::to_string(int_choice));
  }
  return -1;
}

std::string ClientGame::SelectAudio(std::string msg) {
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
    
    drawrer_.PrintCenteredLine(8, utils::ToUpper(msg));
    drawrer_.PrintCenteredLine(10, utils::ToUpper(selector.title_));
    drawrer_.PrintCenteredLine(11, selector.path_);
    drawrer_.PrintCenteredLine(12, help);

    attron(COLOR_PAIR(COLOR_ERROR));
    drawrer_.PrintCenteredLine(13, error);
    error = "";
    attron(COLOR_PAIR(COLOR_DEFAULT));

    for (unsigned int i=0; i<visible_options.size(); i++) {
      if (i == selected) {
        attron(COLOR_PAIR(COLOR_AVAILIBLE));
        attron(WA_BOLD);
      }
      drawrer_.PrintCenteredLine(15 + i, visible_options[i].second);
      attron(COLOR_PAIR(COLOR_DEFAULT));
      attroff(WA_BOLD);
    }

    // Get players choice.
    char choice = getch();
    // Goes to child directory.
    if (utils::IsRight(choice)) {
      level++;
      // Go to "recently played" songs
      if (visible_options[selected].first == "dissonance_recently_played") {
        selector = SetupAudioSelector("", "Recently Played", recently_played);
        selected = 0;
        print_start = 0;
      }
      // Go to child directory (if path exists and is a directory)
      else if (std::filesystem::is_directory(visible_options[selected].first)) {
        selector = SetupAudioSelector(visible_options[selected].first, visible_options[selected].second, 
            utils::GetAllPathsInDirectory(visible_options[selected].first));
        selected = 0;
        print_start = 0;
      }
      // Otherwise: show error.
      else 
        error = "Not a directory!";
    }
    // Go to parent directory
    else if (utils::IsLeft(choice)) {
      // if "top level" reached, then show error.
      if (level == 0) 
        error = "No parent directory.";
      // otherwise go to parent directory (decreases level)
      else {
        level--;
        selected = 0;
        print_start = 0;
        // If "top level" use initial AudioSelector (displaying initial/ customn user directory)
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

void ClientGame::EditSettings() {
  drawrer_.ClearField();
  drawrer_.PrintCenteredLine(LINES/4, "SETTINGS");
  drawrer_.PrintCenteredLine(LINES/4+1, "use 'j'/'k' to cycle selection. Hit '[enter]' to toggle setting");
  auto settings = LoadSettingsJson();
  int cur_selection = 0;
  std::map<int, std::string> settings_mapping = {{0, "stay_in_synapse_menu"}, {1, "show_full_welcome"},
    {2, "music_on"}};
  while(true) {
    if (cur_selection == 0) attron(COLOR_PAIR(COLOR_AVAILIBLE));
    drawrer_.PrintCenteredLine(LINES/4+10, "Stay in synapse-menu when setting applied");
    attron(COLOR_PAIR(COLOR_DEFAULT));
    drawrer_.PrintCenteredLine(LINES/4+11, (settings["stay_in_synapse_menu"]) ? "yes" : "no");

    if (cur_selection == 1) attron(COLOR_PAIR(COLOR_AVAILIBLE));
    drawrer_.PrintCenteredLine(LINES/4+13, "Show full welcome at game-start");
    attron(COLOR_PAIR(COLOR_DEFAULT));
    drawrer_.PrintCenteredLine(LINES/4+14, (settings["show_full_welcome"]) ? "yes" : "no");

    if (cur_selection == 2) attron(COLOR_PAIR(COLOR_AVAILIBLE));
    drawrer_.PrintCenteredLine(LINES/4+16, "Music on");
    attron(COLOR_PAIR(COLOR_DEFAULT));
    drawrer_.PrintCenteredLine(LINES/4+17, (settings["music_on"]) ? "yes" : "no");

    char c = getch();
    if (c == 'q')
      break;
    else if (c == 'j')
      cur_selection = utils::Mod(cur_selection+1, settings_mapping.size());
    else if (c == 'k')
      cur_selection = utils::Mod(cur_selection-1, settings_mapping.size());
    else if (c == '\n') {
      if (settings_mapping.count(cur_selection) > 0) {
        settings[settings_mapping.at(cur_selection)] = !settings[settings_mapping.at(cur_selection)];
      }
    }
    refresh();
  }
  utils::WriteJsonFromDisc(base_path_ + "settings/settings.json", settings);
  LoadSettings();
}

nlohmann::json ClientGame::LoadSettingsJson() {
  return utils::LoadJsonFromDisc(base_path_ + "/settings/settings.json");
}
void ClientGame::LoadSettings() {
  auto settings = LoadSettingsJson();
  stay_in_synapse_menu_ = settings["stay_in_synapse_menu"];
  show_full_welcome_text_ = settings["show_full_welcome"];
  music_on_ = settings["music_on"];
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

void ClientGame::WrapUp() {
  refresh();
  clear();
  endwin();
  exit(0);
}
