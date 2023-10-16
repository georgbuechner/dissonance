#include <filesystem>
#include <math.h>
#include <spdlog/spdlog.h>
#include "client/game/client_game.h"
#include "client/websocket/client.h"
#include "client_game.h"
#include "curses.h"
#include "nlohmann/json.hpp"
#include "share/constants/codes.h"
#include "share/constants/texts.h"
#include "share/objects/units.h"
#include "share/tools/utils/utils.h"
#include "share/shemes/data.h"

#define STD_HANDLERS -1
#define TUTORIAL_MSG_LAG 300

#define CONTEXT_RESOURCES_MSG "Distribute (+)/ remove (-) iron to handler resource-gain"
#define CONTEXT_TECHNOLOGIES_MSG "Research technology by pressing [enter]"
#define CONTEXT_SYNAPSE_MSG "Select instructions for this synapse"

std::map<int, std::map<char, void(ClientGame::*)(std::shared_ptr<Data>)>> ClientGame::handlers_ = {};

void ClientGame::init() {
  handlers_ = {
    { STD_HANDLERS, {
        {'j', &ClientGame::h_MoveSelectionUp}, {'k', &ClientGame::h_MoveSelectionDown}, 
        {'t', &ClientGame::h_ChangeViewPoint}, {'q', &ClientGame::h_Quit}, {'s', &ClientGame::h_SendSelectSynapse}, 
        {'A', &ClientGame::h_BuildNeuron}, {'S', &ClientGame::h_BuildNeuron}, {'N', &ClientGame::h_BuildNeuron}, 
        {'e', &ClientGame::h_BuildPotential}, {'i', &ClientGame::h_BuildPotential}, 
        {'m', &ClientGame::h_BuildPotential}, {'?', &ClientGame::h_Help}, {'r', &ClientGame::h_Refresh}
      }
    },
    { CONTEXT_FIELD, {
        {'j', &ClientGame::h_MoveSelectionUp}, {'k', &ClientGame::h_MoveSelectionDown}, 
        {'h', &ClientGame::h_MoveSelectionLeft}, {'l', &ClientGame::h_MoveSelectionRight}, 
        {'\n', &ClientGame::h_AddPosition}, {'q', &ClientGame::h_ToResourceContext},
        {'?', &ClientGame::h_Help}
      }
    },
    { CONTEXT_RESOURCES, {{'+', &ClientGame::h_AddIron}, {'-', &ClientGame::h_RemoveIron}} },
    { CONTEXT_TECHNOLOGIES, {{'\n', &ClientGame::h_AddTech}} },
    { CONTEXT_SYNAPSE, {
        {'w', &ClientGame::h_SetWPs}, {'i', &ClientGame::h_SetTarget}, {'e', &ClientGame::h_SetTarget}, 
        {'m', &ClientGame::h_SetTarget}, {'s', &ClientGame::h_SwarmAttack}, {'t', &ClientGame::h_ChangeViewPoint}, 
        {'q', &ClientGame::h_ResetOrQuitSynapseContext}
      }
    },
    { CONTEXT_POST_GAME, {
        {'h', &ClientGame::h_MoveSelectionUp}, {'l', &ClientGame::h_MoveSelectionDown},
        {'1', &ClientGame::h_ToggleShowResource}, {'2', &ClientGame::h_ToggleShowResource}, 
        {'3', &ClientGame::h_ToggleShowResource}, {'4', &ClientGame::h_ToggleShowResource}, 
        {'5', &ClientGame::h_ToggleShowResource}, {'6', &ClientGame::h_ToggleShowResource}, 
        {'g', &ClientGame::h_ToggleGraphView}
      }
    },
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

t_topline synapse_topline = {{" set [w]ay-points ", COLOR_DEFAULT}, {" [i]psp-target ", 
  COLOR_DEFAULT}, { " [e]psp-target ", COLOR_DEFAULT}, { " [m]acro-target ", COLOR_DEFAULT}, 
  { " toggle [s]warm-attack ", COLOR_DEFAULT}, {" [t]oggle-navigation ", COLOR_DEFAULT}, 
  {" [h]elp ", COLOR_DEFAULT}, {" [q]uit ", COLOR_DEFAULT}
};

ClientGame::ClientGame(std::string base_path, std::string username, bool mp) : username_(username), 
    muliplayer_availible_(mp), base_path_(base_path), drawrer_(), audio_(base_path),
    audio_data_(base_path_ + USER_FILES) {
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
  init_pair(COLOR_DEFAULT, -1, -1);
  init_pair(COLOR_PAIR_BLUE, COLOR_BLUE, -1);
  init_pair(COLOR_PAIR_GREEN, COLOR_GREEN, -1);
  init_pair(COLOR_PAIR_CYAN, COLOR_CYAN, -1);
  init_pair(COLOR_PAIR_RED, COLOR_RED, -1);
  init_pair(COLOR_PAIR_MAGENTA, COLOR_MAGENTA, -1);
  init_pair(COLOR_PAIR_BROWN, COLOR_PAIR_BROWN, -1);

  init_pair(COLOR_P2, 10, -1); // player 
  init_pair(COLOR_P3, 11, -1); // player 
  init_pair(COLOR_P4, 12, -1); // player 
  init_pair(COLOR_P5, 13, -1); // player

  // Setup map-size
  drawrer_.SetUpBorders(LINES, COLS);

  // Set-up audio-paths.
  std::vector<std::string> paths = utils::LoadJsonFromDisc(base_path_ + MUSIC_PATHS);
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
    handlers_[STD_HANDLERS][' '] = &ClientGame::h_PauseOrUnpause;
    handlers_[CONTEXT_FIELD][' '] = &ClientGame::h_PauseOrUnpause;
    std_topline.push_back({"[space] pause", COLOR_AVAILIBLE});
  }

  current_context_ = CONTEXT_RESOURCES;
  contexts_[CONTEXT_FIELD] = Context("", handlers_[CONTEXT_FIELD], field_topline);
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
  eventmanager_.AddHandler("preparing", &ClientGame::m_Preparing);
  eventmanager_.AddHandler("select_mode", &ClientGame::m_SelectMode);
  eventmanager_.AddHandler("select_audio", &ClientGame::m_SelectAudio);
  eventmanager_.AddHandler("send_audio_data", &ClientGame::m_GetAudioData);
  eventmanager_.AddHandler("audio_exists", &ClientGame::m_SongExists);
  eventmanager_.AddHandler("send_audio_info", &ClientGame::m_SendAudioInfo);
  eventmanager_.AddHandler("print_msg", &ClientGame::m_PrintMsg);
  eventmanager_.AddHandler("init_game", &ClientGame::m_InitGame);
  eventmanager_.AddHandler("update_game", &ClientGame::m_UpdateGame);
  eventmanager_.AddHandler("update_lobby", &ClientGame::m_UpdateLobby);
  eventmanager_.AddHandler("set_msg", &ClientGame::m_SetMsg);
  eventmanager_.AddHandler("game_end", &ClientGame::m_GameEnd);
  eventmanager_.AddHandler("set_unit", &ClientGame::m_SetUnit);
  eventmanager_.AddHandler("set_units", &ClientGame::m_SetUnits);
  eventmanager_.AddHandler("add_technology", &ClientGame::h_AddTechnology);
  eventmanager_.AddHandler("build_neuron", &ClientGame::h_BuildNeuron);
  eventmanager_.AddHandler("build_potential", &ClientGame::h_BuildPotential);
  eventmanager_.AddHandler("select_synapse", &ClientGame::h_SendSelectSynapse);
  eventmanager_.AddHandler("set_wps", &ClientGame::h_SetWPs);
  eventmanager_.AddHandler("set_target", &ClientGame::h_SetTarget);

  eventmanager_tutorial_.AddHandler("init_game", &ClientGame::h_TutorialGetOxygen);
  eventmanager_tutorial_.AddHandler("set_unit", &ClientGame::h_TutorialSetUnit);
  eventmanager_tutorial_.AddHandler("set_units", &ClientGame::h_TutorialScouted);
  eventmanager_tutorial_.AddHandler("build_neuron", &ClientGame::h_TutorialBuildNeuron);
  eventmanager_tutorial_.AddHandler("set_msg", &ClientGame::h_TutorialSetMessage);
  eventmanager_tutorial_.AddHandler("update_game", &ClientGame::h_TutorialUpdateGame);

  tutorial_ = Tutorial({0, 0, 0, true, true, false, false, false, false, false, false, false, false, false});
  drawrer_.LoadMiniField(base_path_ + MINI_FIELDS_PATH);

  LoadRanking();
}

// setter 
void ClientGame::set_client(Client* ws_srv) {
  ws_srv_ = ws_srv;
}

void ClientGame::HandleAction(Command cmd) {
  spdlog::get(LOGGER)->debug("ClientGame::HandleAction: command: {}, context: {}", cmd.command(), current_context_);
  // Get event from event-manager
  if (eventmanager_.handlers().count(cmd.command()))
    (this->*eventmanager_.handlers().at(cmd.command()))(cmd.data());
  if (mode_ == TUTORIAL && eventmanager_tutorial_.handlers().count(cmd.command()) > 0)
    (this->*eventmanager_tutorial_.handlers().at(cmd.command()))(cmd.data());
}

void ClientGame::GetAction() {
  spdlog::get(LOGGER)->info("ClientGame::GetAction stated");
  while(true) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    // Get Input
    if (status_ == WAITING) continue; // Skip as long as not active.
    char choice = getch();
    history_.push_back(choice);
    spdlog::get(LOGGER)->info("ClientGame::GetAction action_ status: {}, choice: {}, context: {}", 
        status_, choice, current_context_);
    if (status_ == WAITING)
      continue; // Skip as long as not active. 
    // End Postgame
    if (current_context_ == CONTEXT_POST_GAME && choice == 'q')
      break;

    // Throw event
    std::shared_lock sl(mutex_context_);
    if (contexts_.at(current_context_).eventmanager().handlers().count(choice) > 0) {
      auto data = contexts_.at(current_context_).data();
      // Set (entered) command of current context to choice (equivalent to last history entry)
      contexts_.at(current_context_).set_cmd(choice);
      sl.unlock();
      (this->*contexts_.at(current_context_).eventmanager().handlers().at(choice))(data);
    }
    else if (mode_ == TUTORIAL && tutorial_.action_active_ && choice == 'y') {
      h_TutorialAction(std::make_shared<Data>());
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

void ClientGame::Kill(std::string msg) {
  h_Kill(std::make_shared<Msg>(msg));
}

void ClientGame::h_Kill(std::shared_ptr<Data> data) {
  drawrer_.set_stop_render(true);
  drawrer_.ClearField();
  drawrer_.PrintCenteredLine(LINES/2, data->msg());
  drawrer_.PrintCenteredLine(LINES/2+2, "[Press any key to leave game]");
  refresh();
  getch();
  // Wrap up and exit.
  WrapUp();
}

void ClientGame::h_Help(std::shared_ptr<Data>) {
  if (mode_ == TUTORIAL || mode_ == SINGLE_PLAYER)
    Pause();
  drawrer_.set_stop_render(true);
  drawrer_.PrintCenteredParagraphs(texts::help);
  drawrer_.set_stop_render(false);
  if (mode_ == TUTORIAL)
    UnPause();
}

void ClientGame::h_Quit(std::shared_ptr<Data>) {
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

void ClientGame::h_Refresh(std::shared_ptr<Data> data) {
  drawrer_.ClearField();
}

void ClientGame::h_PauseOrUnpause(std::shared_ptr<Data>) {
  if (pause_)
    UnPause();
  else 
    Pause();
}

void ClientGame::h_MoveSelectionUp(std::shared_ptr<Data>) {
  drawrer_.inc_cur_sidebar_elem(1);
}

void ClientGame::h_MoveSelectionDown(std::shared_ptr<Data>) {
  drawrer_.inc_cur_sidebar_elem(-1);
}

void ClientGame::h_MoveSelectionLeft(std::shared_ptr<Data>) {
  drawrer_.inc_cur_sidebar_elem(-2);
}

void ClientGame::h_MoveSelectionRight(std::shared_ptr<Data>) {
  drawrer_.inc_cur_sidebar_elem(2);
}

void ClientGame::h_SelectGame(std::shared_ptr<Data>) {
  spdlog::get(LOGGER)->info("ClientGame::h_SelectGame");
  // Create data to send.
  std::shared_ptr<Data> data = std::make_shared<InitNewGame>(MULTI_PLAYER_CLIENT, drawrer_.field_height(), 
      drawrer_.field_width());
  // Get and check game-id.
  std::string game_id = drawrer_.game_id_from_lobby();
  if (game_id != "") {
    data->set_game_id(game_id);
    ws_srv_->SendMessage("setup_new_game", data);
    current_context_ = CONTEXT_RESOURCES;
    drawrer_.set_viewpoint(current_context_);
  }
  else {
    drawrer_.set_msg("Game probably no longer availible");
  }
}

void ClientGame::h_ChangeViewPoint(std::shared_ptr<Data>) {
  drawrer_.ClearMarkers();
  current_context_ = drawrer_.next_viewpoint();
  drawrer_.set_msg(contexts_.at(current_context_).msg());
  drawrer_.set_topline(contexts_.at(current_context_).topline());
}

void ClientGame::h_AddIron(std::shared_ptr<Data>) {
  ws_srv_->SendMessage("add_iron", std::make_shared<DistributeIron>(drawrer_.GetResource()));
}

void ClientGame::h_RemoveIron(std::shared_ptr<Data>) {
  ws_srv_->SendMessage("remove_iron", std::make_shared<DistributeIron>(drawrer_.GetResource()));
}

void ClientGame::h_AddTech(std::shared_ptr<Data>) {
  ws_srv_->SendMessage("add_technology", std::make_shared<AddTechnology>(drawrer_.GetTech()));
}

void ClientGame::h_BuildPotential(std::shared_ptr<Data> data) {
  // final call: send request to create potentials
  if (data->synapse_pos() != DEFAULT_POS) {
    RemovePickContext(CONTEXT_RESOURCES);
    ws_srv_->SendMessage("check_build_potential", data);
  }
  // second call: select synapse
  else if (data->positions().size() > 0) {
    SwitchToPickContext(data->positions(), "Select synapse", "build_potential", data);
  }
  // first call: set potential-type and number of potentials to build
  else {
    int num = 1;
    // If history is given and (second) last history entry is digit, create that number of potentials.
    if (history_.size() > 1 && std::isdigit(history_[history_.size()-2])) 
      num = history_[history_.size()-2] - '0';
    // In case call from server has invalid entries and results to this case.
    if (char_unit_mapping.count(contexts_.at(current_context_).cmd()) == 0) {
      spdlog::get(LOGGER)->warn("ClientGame::h_BuildPotential (1) probably not called from `GetAction`, cmd: {}",
          contexts_.at(current_context_).cmd());
      return;
    }
    int unit = char_unit_mapping.at(contexts_.at(current_context_).cmd());
    ws_srv_->SendMessage("check_build_potential", std::make_shared<BuildPotential>(unit, num));
  }
}

void ClientGame::h_ToResourceContext(std::shared_ptr<Data>) {
  SwitchToResourceContext("Aborted adding neuron/ potential");
}

void ClientGame::h_BuildNeuron(std::shared_ptr<Data> data) {
  // final call: target build-position set, send message to server to build neuron.
  if (data->pos() != DEFAULT_POS) {
    ws_srv_->SendMessage("build_neuron", data);
  }
  // second call with start-position, or third call with start position from marker: select pos from field
  else if (data->start_pos() != DEFAULT_POS) {
    RemovePickContext();
    SwitchToFieldContext(data->start_pos(), (data->unit() == NUCLEUS) ? 999 : data->range(),
        "build_neuron", data, "Select position to build " + units_tech_name_mapping.at(data->unit()));
  }
  // second call no start-position: Add Pick-Context to select start position
  else if (data->positions().size() > 0) {
    std::string msg = (data->unit() == NUCLEUS) ? "Select start-position" : "Select nucleus";
    SwitchToPickContext(data->positions(), msg, "build_neuron", data);
  }
  // first call: send message to server, checking wether neuron can be build.
  else {
    // In case call from server has invalid entries and results to this case.
    if (char_unit_mapping.count(contexts_.at(current_context_).cmd()) == 0) {
      spdlog::get(LOGGER)->warn("ClientGame::h_BuildNeuron (1) probably not called from `GetAction`, cmd: {}",
          contexts_.at(current_context_).cmd());
      return;
    }
    int unit = char_unit_mapping.at(contexts_.at(current_context_).cmd());
    ws_srv_->SendMessage("check_build_neuron", std::make_shared<BuildNeuron>(unit));
  }
}

void ClientGame::h_SendSelectSynapse(std::shared_ptr<Data> data) {
  if (data->synapse_pos() != DEFAULT_POS) {
    RemovePickContext();
    SwitchToSynapseContext(data);
    drawrer_.set_msg("Choose what to do.");
    drawrer_.AddMarker(SYNAPSE_MARKER, data->synapse_pos(), COLOR_MARKED);
  }
  // second call: SelectSynapse-Class has been initialized. 
  // select synapse-pos from positions (or print error, if zero positions)
  else if (data->called()) {
    if (data->player_units().size() == 0) {
      drawrer_.set_msg("No synapse.");
      SwitchToResourceContext();
    }
    else
      SwitchToPickContext(data->player_units(), "Select synapse", "select_synapse", data);
  }
  // first call: initalize synapse-context
  else {
    SwitchToSynapseContext();
    std::map<int, Data::PositionInfo> position_requests = {{{PLAYER, Data::PositionInfo(SYNAPSE)}}};
    ws_srv_->SendMessage("get_positions", 
        std::make_shared<GetPositions>("select_synapse", position_requests, std::make_shared<SelectSynapse>()));
  }
}

void ClientGame::h_SetWPs(std::shared_ptr<Data> data) {
  // Technology not researched
  if (!drawrer_.synapse_options().at(0)) {
    FinalSynapseContextAction(data->synapse_pos());
    drawrer_.set_msg("Technology \"choose way\" not researched");
    return;
  }
  // Final call: Send request to set way-point.
  if (data->way_point() != DEFAULT_POS) {
    ws_srv_->SendMessage("set_way_point", data);
  }
  // third call: select field position
  else if (data->start_pos() != DEFAULT_POS) {
    RemovePickContext(CONTEXT_SYNAPSE);
    SwitchToFieldContext(data->start_pos(), FULL_MAP_ACCESSS, "set_wps", data, "Select new way-point position.", {'q'});
  }
  // second call: select start position
  else if (data->centered_positions().size() > 0) {
    // print ways:
    for (const auto& it : data->current_way())
      drawrer_.AddMarker(WAY_MARKER, it, COLOR_MARKED);
    // print current-waypoints 
    for (size_t i=0; i<data->current_waypoints().size(); i++)
      drawrer_.AddMarker(WAY_MARKER, data->current_waypoints()[i], COLOR_MARKED, 
          utils::CharToString('1', i));
    // set up pick-context:
    SwitchToPickContext(data->centered_positions(), "select start position", "set_wps", data, {'q'});
  }
  // First call (request positions)
  else {
    // If "msg" is contained, print message
    if (data->msg() != "")
      drawrer_.set_msg(data->msg());
    // If num is not set (-1), turn data into a SetWayPoints object.
    if (data->num() == -1) {
      data = std::make_shared<SetWayPoints>(data->synapse_pos());
    }
    // If num is 0xFFF, this indicates, that no more way-point can be set: switch
    // to resource-context or stay at synapse-context depending on settings.
    if (data->num() == 0xFFF)
      FinalSynapseContextAction(data->synapse_pos());
    // Otherwise, request inital data.
    else {
      // Request inital data.
      std::map<int, Data::PositionInfo> position_requests = {
        {CURRENT_WAY, Data::PositionInfo(data->synapse_pos())}, 
        {CURRENT_WAY_POINTS, Data::PositionInfo(data->synapse_pos())}, {CENTER, Data::PositionInfo()}
      };
      ws_srv_->SendMessage("get_positions", std::make_shared<GetPositions>("set_wps", position_requests, data));
    }
  }
}

void ClientGame::h_SetTarget(std::shared_ptr<Data> data) {
  // final call: send request to server, setting target and reset to synapse-context
  if (data->target() != DEFAULT_POS) {
    FinalSynapseContextAction(data->synapse_pos());
    ws_srv_->SendMessage("set_target", data);
  }
  // third call: select field position
  else if (data->start_pos() != DEFAULT_POS) {
    RemovePickContext(CONTEXT_SYNAPSE);
    SwitchToFieldContext(data->start_pos(), FULL_MAP_ACCESSS, "target", data, "Select target position.", {'q'});
  }
  // Second call: select start position (first pos, or via pick-context) and show current-targets.
  else if (data->enemy_units().size() > 0 || data->target_positions().size()) {
    // Only go into pick-context if more than 1 enemy-nucleus.
    if (data->enemy_units().size() > 1)
      SwitchToPickContext(data->enemy_units(), "Select select enemy base", "target", data, {'q'});
    // Only mark target-positions on map if target-positions are included.
    if (data->target_positions().size() > 0)
      drawrer_.AddMarker(TARGETS_MARKER, data->target_positions().front(), COLOR_MARKED, "T");
  }
  // First call: request positions
  else {
    auto cmd = contexts_.at(current_context_).cmd();
    int unit = (cmd == 'e') ? EPSP : (cmd == 'i') ? IPSP : MACRO;
    std::map<int, Data::PositionInfo> position_requests = {{ENEMY, Data::PositionInfo(NUCLEUS)}, 
        {TARGETS, Data::PositionInfo(unit, data->synapse_pos())}};
    std::shared_ptr<Data> inital_set_target = std::make_shared<SetTarget>(data->synapse_pos(), unit);
    ws_srv_->SendMessage("get_positions", 
        std::make_shared<GetPositions>("set_target", position_requests, inital_set_target));
  }
}

void ClientGame::h_SwarmAttack(std::shared_ptr<Data> data) {
  // Depending on setting: switch to resource context, then send message
  FinalSynapseContextAction(data->synapse_pos());
  ws_srv_->SendMessage("toggle_swarm_attack", data);
}

void ClientGame::h_ResetOrQuitSynapseContext(std::shared_ptr<Data> data) {
  // if current context if not already synapse-context: switch to synapse-context.
  if (current_context_ != CONTEXT_SYNAPSE) {
    SwitchToSynapseContext(data);
    drawrer_.set_viewpoint(CONTEXT_RESOURCES);
    drawrer_.ClearMarkers();
    drawrer_.AddMarker(SYNAPSE_MARKER, data->synapse_pos(), COLOR_MARKED);
  }
  else {
    SwitchToResourceContext();
  }
}

void ClientGame::h_ToggleGraphView(std::shared_ptr<Data>) {
  drawrer_.ToggleGraphView();
}

void ClientGame::h_ToggleShowResource(std::shared_ptr<Data>) {
  char cmd = contexts_.at(current_context_).cmd();
  int resouce = cmd - '0';
  drawrer_.ToggleShowResource(resouce);
}

void ClientGame::h_AddPosition(std::shared_ptr<Data> data) {
  // Check position and print message if position not free or not in graph.
  std::string action = contexts_.at(current_context_).action();
  if (!drawrer_.InGraph(drawrer_.field_pos())) {
    drawrer_.set_msg("Position not reachable!");
    return;
  }
  if (action == "build_neuron" && !drawrer_.Free(drawrer_.field_pos())) {
    drawrer_.set_msg("Position not free!");
    return;
  }
  // Set position
  data->SetPickedPosition(drawrer_.field_pos());
  // Call action stored in context.
  if (action == "build_neuron")
    h_BuildNeuron(data);
  else if (action == "set_wps") 
    h_SetWPs(data);
  else if (action == "target") 
    h_SetTarget(data);
}

void ClientGame::h_AddStartPosition(std::shared_ptr<Data> data) {
  // Set position
  data->SetPickedPosition(drawrer_.GetMarkerPos(PICK_MARKER, utils::CharToString(history_.back(), 0)));
  // Call action 
  std::string action = contexts_.at(current_context_).action();
  if (action == "build_potential")
    h_BuildPotential(data);
  else if (action == "build_neuron")
    h_BuildNeuron(data);
  else if (action == "select_synapse") 
    h_SendSelectSynapse(data);
  else if (action == "set_wps") 
    h_SetWPs(data);
  else if (action == "target") 
    h_SetTarget(data);
}

void ClientGame::SwitchToResourceContext(std::string msg) {
  std::shared_lock sl(mutex_context_);
  current_context_ = CONTEXT_RESOURCES;
  drawrer_.set_topline(contexts_.at(current_context_).topline());
  drawrer_.set_viewpoint(current_context_);
  drawrer_.ClearMarkers();
  if (msg != "")
    drawrer_.set_msg(msg);
}

void ClientGame::SwitchToSynapseContext(std::shared_ptr<Data> data) {
  std::shared_lock sl(mutex_context_);
  current_context_ = CONTEXT_SYNAPSE;
  contexts_.at(current_context_).set_data(data);
  drawrer_.set_topline(contexts_.at(current_context_).topline());
}

void ClientGame::FinalSynapseContextAction(position_t synapse_pos) {
  // Switch (back) to synapse-context (clear only target-markers, leave synapse marked)
  if (stay_in_synapse_menu_ && synapse_pos != DEFAULT_POS) {
    SwitchToSynapseContext(std::make_shared<SelectSynapse>(synapse_pos));
    drawrer_.ClearMarkers(TARGETS_MARKER);
    drawrer_.ClearMarkers(WAY_MARKER);
  }
  // End synapse-context and go to resource-context (clear ALL markers).
  else {
    SwitchToResourceContext();
  }
  drawrer_.set_viewpoint(CONTEXT_RESOURCES);
}

void ClientGame::SwitchToPickContext(std::vector<position_t> positions, std::string msg, 
    std::string action, std::shared_ptr<Data> data, std::vector<char> slip_handlers) {
  std::shared_lock sl(mutex_context_);
  // Get all handlers and add markers to drawrer.
  drawrer_.ClearMarkers(PICK_MARKER);
  std::map<char, void(ClientGame::*)(std::shared_ptr<Data>)> new_handlers = {{'t', &ClientGame::h_ChangeViewPoint}};
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

void ClientGame::SwitchToFieldContext(position_t pos, int range, std::string action, std::shared_ptr<Data> data,
    std::string msg, std::vector<char> slip_handlers) {
  std::shared_lock sl(mutex_context_);
  // Set data for field-context and empy old positions.
  contexts_.at(CONTEXT_FIELD).set_action(action);
  contexts_.at(CONTEXT_FIELD).set_data(data);
  contexts_.at(CONTEXT_FIELD).set_handlers(handlers_[CONTEXT_FIELD]);
  // Let handlers slip through.
  for (const auto& it : slip_handlers) {
    if (contexts_.at(current_context_).eventmanager().handlers().count(it) > 0)
      contexts_.at(CONTEXT_FIELD).eventmanager().AddHandler(it, 
          contexts_.at(current_context_).eventmanager().handlers().at(it));
  }
  current_context_ = CONTEXT_FIELD;

  // Set up drawrer/
  drawrer_.set_field_start_pos(pos);
  drawrer_.set_range({pos, range});
  drawrer_.set_viewpoint(current_context_);
  drawrer_.set_topline(contexts_.at(current_context_).topline());
  drawrer_.set_msg(msg);
}

void ClientGame::RemovePickContext(int new_context) {
  std::shared_lock sl(mutex_context_);
  // Delete pick-context if exists.
  if (contexts_.count(CONTEXT_PICK) > 0)
    contexts_.erase(CONTEXT_PICK);
  // Clear all markers 
  drawrer_.ClearMarkers();
  // If new context is given, switch to given context
  if (new_context != -1) {
    current_context_ = new_context;
    drawrer_.set_viewpoint(current_context_);
    drawrer_.set_topline(contexts_.at(current_context_).topline());
  }
}

void ClientGame::m_Preparing(std::shared_ptr<Data>) {
  // Waiting is only to overwrite ugly debug-output from aubio (only single-player).
  if (!muliplayer_availible_)
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  drawrer_.PrintOnlyCenteredLine(LINES/2, "Analyzing audio...");
}

void ClientGame::m_SelectMode(std::shared_ptr<Data> data) {
  // Print welcome text.
  auto welcome_text = (show_full_welcome_text_) ? texts::welcome : texts::welcome_reduced;
  // Append server-message.
  if (data->paragraph().size() > 0) {
    welcome_text.push_back(data->paragraph());
  }
  drawrer_.PrintCenteredParagraphs(welcome_text);
  
  // Select single-player, mulit-player (host/ client), observer.
  choice_mapping_t mapping = {
    {SINGLE_PLAYER, {"singe-player", (!muliplayer_availible_) ? COLOR_AVAILIBLE : COLOR_DEFAULT}}, // only in sp
    {MULTI_PLAYER, {"muli-player (host)", (muliplayer_availible_) ? COLOR_AVAILIBLE : COLOR_DEFAULT}}, 
    {MULTI_PLAYER_CLIENT, {"muli-player (client)", (muliplayer_availible_) ? COLOR_AVAILIBLE : COLOR_DEFAULT}}, 
    {TUTORIAL, {"tutorial", (!muliplayer_availible_) ? COLOR_AVAILIBLE : COLOR_DEFAULT}}, // only in sp
    {OBSERVER, {"watch ki", (!muliplayer_availible_) ? COLOR_AVAILIBLE : COLOR_DEFAULT}}, // only in sp
    {SETTINGS, {"settings", COLOR_AVAILIBLE}},
    {RANKING, {"show local ranking", COLOR_AVAILIBLE}}
  };
  mode_ = SelectInteger("Select mode", true, mapping, {mapping.size()+1}, "Mode not available");

  // If mode is settings, let player edit settings (game closed afterwards!!)
  if (mode_ == SETTINGS)
    EditSettings(); // closes game!!
  if (mode_ == RANKING)
    PrintRanking();
                    
  drawrer_.set_mode(mode_);
  auto new_data = std::make_shared<InitNewGame>(mode_, drawrer_.field_height(), drawrer_.field_width());
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
    int num_players = 2 + SelectInteger("Select number of players", true, mapping, {mapping.size()+1}, "Max 4 players!");
    new_data->set_num_players(num_players);
  }
  ws_srv_->SendMessage("setup_new_game", new_data);
}

void ClientGame::m_SelectAudio(std::shared_ptr<Data>) {
  // Load map-audio.
  audio_file_path_ = SelectAudio("select map");
  std::filesystem::path p = audio_file_path_;
  std::string filename = p.filename().string();
  std::shared_ptr<CheckSendAudio> new_data = nullptr;
  if (mode_ != MULTI_PLAYER) {
    new_data = std::make_shared<CheckSendAudio>(audio_file_path_);
    // If no ranking-entry for song exists, create one.
    std::string ranking_entry_id = RankingEntry::id(audio_file_path_, {drawrer_.field_height(), drawrer_.field_width()});
    if (ranking_.count(ranking_entry_id) == 0) {
      ranking_[ranking_entry_id] = new RankingEntry(filename, {drawrer_.field_width(), drawrer_.field_height()});
    }
  }
  else {
    new_data = std::make_shared<CheckSendAudio>(username_ + "/" + filename, filename);
  }
  ws_srv_->SendMessage("audio_map", new_data);
}

void ClientGame::m_GetAudioData(std::shared_ptr<Data> data) {
  audio_data_.AddData(data);
  if (audio_data_.audio_stored()) {
    ws_srv_->SendMessage("ready", std::make_shared<Data>());
    audio_file_path_ = audio_data_.audio_file_path();
    audio_data_.Clear(); // audio will be loaded from disc, no need to keep in memory here.
  }
}

void ClientGame::m_SongExists(std::shared_ptr<Data> data) {
  std::string path = base_path_ + USER_FILES + data->map_path();
  ws_srv_->SendMessage((!std::filesystem::exists(path)) ? "send_audio" : "ready", std::make_shared<Data>());
}

void ClientGame::m_SendAudioInfo(std::shared_ptr<Data> data) {
  // If server does not have song, then send song.
  if (data->send_audio())
    SendSong();
  // Add map-file name.
  std::filesystem::path p = audio_file_path_;
  auto new_data = std::make_shared<InitializeGame>(p.filename().string());

  // If observer, load audio for two ai-files (no need to send audio-file: only analysed data)
  if (data->send_ai_audios()) {
    for (int i = 0; i<2; i++) {
      std::string source_path = SelectAudio("select ai sound " + std::to_string(i+1));
      Audio audio(base_path_);
      audio.set_source_path(source_path);
      audio.Analyze();
      new_data->AddAiAudioData(source_path, audio.GetAnalyzedData());
    }
  }
  ws_srv_->SendMessage("initialize_game", new_data);
}

void ClientGame::SendSong() {
  // Create initial data
  std::filesystem::path p = audio_file_path_;
  auto data = std::make_shared<AudioTransferData>(username_, p.filename().string());
  // Get content.
  audio_data_.set_audio_data(utils::GetMedia(audio_file_path_));
  auto chunks = audio_data_.GetCunks();
  data->set_parts(chunks.size());
  for (const auto& it : chunks) {
    // Update dto.
    data->set_part(it.first);
    data->set_content(it.second);
    // Print "progress"-message 
    std::string msg = "Sending audio part " + std::to_string(it.first+1) + " of " 
      + std::to_string(chunks.size()) + "...";
    if ((size_t)it.first+1 == chunks.size())
      msg = "Data sent. Waiting for server...";
    drawrer_.PrintOnlyCenteredLine(LINES/2, msg);
    // Try sending chunk to server.
    try {
      ws_srv_->SendMessage("send_audio_data", data);
    } 
    // Otherwise (file size to big) print error message and ask for user-input (confirmation)
    catch(...) {
      drawrer_.ClearField();
      drawrer_.PrintCenteredLine(LINES/2, "File size too big! (press any key to continue.)");
      getch();
      return;
    }
  }
}

void ClientGame::m_PrintMsg(std::shared_ptr<Data> data) {
  drawrer_.PrintOnlyCenteredLine(LINES/2, data->msg());
}

void ClientGame::m_InitGame(std::shared_ptr<Data> data) {
  spdlog::get(LOGGER)->debug("ClientGame::m_InitGame");
  std::unique_lock ul(mutex_);
  // Play audio right away, since game has already begun.
  audio_.set_source_path(audio_file_path_);
  if (music_on_)
    audio_.play();
  // Setup drawrer with transfer-data.
  drawrer_.ClearField();
  drawrer_.set_transfer(data, show_ai_tactics_);
  drawrer_.PrintGame(false, false, current_context_);
  status_ = RUNNING;
  // Setup context and print topline.
  current_context_ = CONTEXT_RESOURCES;
  // Overwrite macro-information-message if show_ai_tactics_
  drawrer_.set_topline(contexts_.at(current_context_).topline());
  spdlog::get(LOGGER)->debug("ClientGame::m_InitGame: done");
}

void ClientGame::m_UpdateGame(std::shared_ptr<Data> data) {
  std::unique_lock ul(mutex_);
  drawrer_.UpdateTranser(data);
  drawrer_.PrintGame(false, false, current_context_);
}

void ClientGame::m_UpdateLobby(std::shared_ptr<Data> data) {
  // Set
  if (current_context_ != VP_LOBBY) {
    status_ = RUNNING;
    current_context_ = CONTEXT_LOBBY;
    drawrer_.set_viewpoint(current_context_);
  }
  // Update Lobby
  drawrer_.UpdateLobby(data);
  drawrer_.PrintLobby();
}

void ClientGame::m_SetMsg(std::shared_ptr<Data> data) {
  drawrer_.set_msg(data->msg());
}

void ClientGame::m_GameEnd(std::shared_ptr<Data> data) {
  drawrer_.set_stop_render(true);
  drawrer_.ClearField();
  drawrer_.PrintCenteredLine(LINES/2, data->msg());
  int counter = 2;
  if (mode_ == TUTORIAL) {
    drawrer_.PrintCenteredLine(LINES/2+counter, "You finished the tutorial! You may want to replay to "
        "explore all hints, we provide.");
    counter +=2;
  }
  drawrer_.PrintCenteredLine(LINES/2+counter, " [Press 'q' to leave game and 'h'/'l' to cycle statistics]");
  drawrer_.set_statistics(data->statistics());
  refresh();
  ws_srv_->Stop();
  std::unique_lock ul(mutex_context_);
  current_context_ = CONTEXT_POST_GAME;
  drawrer_.set_viewpoint(VP_POST_GAME);
  if (mode_ == SINGLE_PLAYER) {
    std::string ranking_entry_id = RankingEntry::id(audio_file_path_, {drawrer_.field_height(), drawrer_.field_width()});
    spdlog::get(LOGGER)->debug("ClientGame::m_GameEnd: ranking_entry_id: {}", ranking_entry_id);
    if (data->msg().find("times up") != std::string::npos)
      ranking_[ranking_entry_id]->IncDraw();
    else if (data->msg().find("LOST") != std::string::npos)
      ranking_[ranking_entry_id]->IncLost();
    else
      ranking_[ranking_entry_id]->IncWon();
    StoreRanking();
  }
}

void ClientGame::m_SetUnit(std::shared_ptr<Data> data) {
  drawrer_.AddNewUnitToPos(data->pos(), data->unit(), data->color());
  SwitchToResourceContext("Success!");
  drawrer_.PrintGame(false, false, current_context_);
}

void ClientGame::m_SetUnits(std::shared_ptr<Data> data) {
  for (const auto& it : data->units()) 
    drawrer_.AddNewUnitToPos(it.pos(), it.unit(), it.color());
  drawrer_.PrintGame(false, false, current_context_);
}

void ClientGame::h_AddTechnology(std::shared_ptr<Data> data) {
  drawrer_.AddTechnology(data->technology());
}

// tutorial 

void ClientGame::h_TutorialGetOxygen(std::shared_ptr<Data>) {
  h_InitTutorialText(texts::tutorial_get_oxygen);
  eventmanager_tutorial_.RemoveHandler("init_game");
}

void ClientGame::h_TutorialSetUnit(std::shared_ptr<Data> data) {
  std::vector<texts::paragraphs_field_t> texts;
  // Get text ad do potential other actions
  if (data->unit() == RESOURCENEURON && data->resource() == OXYGEN && !tutorial_.oxygen_) {
    texts.push_back(texts::tutorial_get_glutamat);
    tutorial_.oxygen_ = true;
  }
  else if (data->unit() == RESOURCENEURON && data->resource() == GLUTAMATE && !tutorial_.glutamate_) {
    texts.push_back(texts::tutorial_build_activated_neurons);
    tutorial_.glutamate_ = true;
  }
  else if (data->unit() == RESOURCENEURON && data->resource() == POTASSIUM && !tutorial_.potassium_) {
    texts.push_back(texts::tutorial_build_synapse);
    tutorial_.potassium_ = true;
  }
  else if (data->unit() == RESOURCENEURON && data->resource() == CHLORIDE && !tutorial_.chloride_) {
    texts.push_back(texts::tutorial_build_ipsp);
    tutorial_.chloride_ = true;
  }
  else if (data->unit() == RESOURCENEURON && data->resource() == DOPAMINE && !tutorial_.dopamine_) {
    texts.push_back(texts::tutorial_technologies_dopamine);
    tutorial_.dopamine_ = true;
    if (tutorial_.serotonin_)
      texts.push_back(texts::tutorial_technologies_all);
    // Always add how-to
    texts.push_back(texts::tutorial_technologies_how_to);
  }
  else if (data->unit() == RESOURCENEURON && data->resource() == SEROTONIN && !tutorial_.serotonin_) {
    texts.push_back(texts::tutorial_technologies_seretonin);
    tutorial_.serotonin_ = true;
    if (tutorial_.dopamine_) {
      texts.push_back(texts::tutorial_technologies_all);
      texts.push_back(texts::tutorial_technologies_how_to);
    }
  }
  else if (data->unit() == ACTIVATEDNEURON && tutorial_.activated_neurons_ == 3) {
    texts.push_back(texts::tutorial_get_potassium);
  }
  else if (data->unit() == ACTIVATEDNEURON && tutorial_.activated_neurons_ == 4) {
    texts.push_back(texts::tutorial_bound_resources);
  }
  else if (data->unit() == SYNAPSE && tutorial_.synapses_ == 0) {
    texts.push_back(texts::tutorial_build_potential);
    tutorial_.synapses_++;
  }
  // If text was set, print text:
  if (texts.size() > 0) {
    // Add all texts.
    auto final_text = texts[0];
    for (unsigned int i=1; i<texts.size(); i++) {
      final_text.insert(final_text.end(), std::make_move_iterator(texts[i].begin()), 
        std::make_move_iterator(texts[i].end()));
    }
    h_InitTutorialText(final_text);
  }
}

void ClientGame::h_TutorialScouted(std::shared_ptr<Data> data) {
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
    for (const auto& it : data->units())
      if (it.unit() == ACTIVATEDNEURON) 
        counter++;
    if (counter >= 2 && tutorial_.chloride_ && tutorial_.dopamine_)  {
      text = texts::tutorial_final_attack;
      tutorial_.action_active_ = true;
      tutorial_.action_ = 0;
      eventmanager_tutorial_.RemoveHandler("set_units");
    }
  }
  // If text was set, print text:
  if (text.size() > 0) {
    h_InitTutorialText(text);
  }
}

void ClientGame::h_TutorialBuildNeuron(std::shared_ptr<Data> data) {
  texts::paragraphs_field_t text;
  // Get text ad do potential other actions
  if (data->unit() == ACTIVATEDNEURON && tutorial_.activated_neurons_ == 0) {
    text = texts::tutorial_first_build;
    tutorial_.activated_neurons_++;
  }
  else if (data->unit() == ACTIVATEDNEURON)
    tutorial_.activated_neurons_++;
 
  // If text was set, print text:
  if (text.size() > 0) {
    h_InitTutorialText(text);
  }
}

void ClientGame::h_TutorialAction(std::shared_ptr<Data>) {
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
    h_InitTutorialText(text);
  }
}

void ClientGame::h_TutorialSetMessage(std::shared_ptr<Data> data) {
  texts::paragraphs_field_t text;
  // Get text ad do potential other actions
  if (data->msg() == "Target for this synapse set" && !tutorial_.epsp_target_set_) {
    text = texts::tutorial_strong_attack;
    tutorial_.epsp_target_set_ = true;
  }
  // If text was set, print text:
  if (text.size() > 0) {
    h_InitTutorialText(text);
  }
}

void ClientGame::h_TutorialUpdateGame(std::shared_ptr<Data> data) {
  texts::paragraphs_field_t text;
  // first enemy attack-launch
  if (data->potentials().size() > 0 && tutorial_.first_attack_) {
    text = texts::tutorial_first_attack;
    tutorial_.first_attack_ = false;
  }
  if (data->players().at(username_).first.front() != '0' && tutorial_.first_damage_) {
    text = texts::tutorial_first_damage;
    tutorial_.first_damage_ = false;
  }
  // If text was set, print text:
  if (text.size() > 0) {
    h_InitTutorialText(text);
  }
}

// text
void ClientGame::h_TextNext(std::shared_ptr<Data>) {
  // Check if last text was reached.
  if (contexts_.at(current_context_).last_text())
    h_TextQuit();
  // Otherwise increase num and then print text.
  else {
    int num = contexts_.at(CONTEXT_TEXT).num();
    contexts_.at(CONTEXT_TEXT).set_num(num+1);
    h_TextPrint();
  }
}

void ClientGame::h_TextPrev(std::shared_ptr<Data>) {
  int num = contexts_.at(CONTEXT_TEXT).num();
  if (num == 0) 
    return;
  contexts_.at(CONTEXT_TEXT).set_num(num-1);
  h_TextPrint();
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
  std::vector<std::string> recently_played = utils::LoadJsonFromDisc(base_path_ + RECENTLYPLAYER_PATH);
  std::string error = "";
  std::string help = "(use \"+\" to add paths, ENTER to select, \"h/l\" to change directory, "
    "\"j/k\" to circle through songs and \"g\" to jump to the top.)";
  size_t selected = 0;
  int level = 0;
  int print_start = 0;
  size_t max = LINES/2;
  std::vector<std::pair<std::string, std::string>> visible_options;
  std::map<std::string, std::pair<int, int>> selected_history;

  while(true) {
    size_t print_max = std::min(selector.options_.size(), max);
    visible_options = utils::SliceVector(selector.options_, print_start, print_max);
    
    drawrer_.PrintCenteredLine(8, utils::ToUpper(msg));
    drawrer_.PrintCenteredLine(10, utils::ToUpper(selector.title_));
    drawrer_.PrintCenteredLine(11, selector.path_);
    drawrer_.PrintCenteredLine(12, help);

    attron(COLOR_PAIR(COLOR_ERROR));
    drawrer_.PrintCenteredLine(13, error);
    error = "";
    attron(COLOR_PAIR(COLOR_DEFAULT));

    for (size_t i=0; i<visible_options.size(); i++) {
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
    if (choice == 'l') {
      if (visible_options[selected].first == "dissonance_recently_played" || 
          std::filesystem::is_directory(visible_options[selected].first)) {
        level++;
        selected_history[selector.path_] = {selected, print_start};
        // Go to "recently played" songs
        if (visible_options[selected].first == "dissonance_recently_played") {
          selector = SetupAudioSelector("recently_played", "Recently Played", recently_played);
        }
        // Go to child directory (if path exists and is a directory)
        else if (std::filesystem::is_directory(visible_options[selected].first)) {
          selector = SetupAudioSelector(visible_options[selected].first, visible_options[selected].second, 
              utils::GetAllPathsInDirectory(visible_options[selected].first));
        }
        if (selected_history.count(selector.path_) > 0) {
          selected = selected_history.at(selector.path_).first;
          print_start = selected_history.at(selector.path_).second;
        }
        else {
          selected = print_start = 0;
        }
      }
      // Otherwise: show error.
      else {
        error = "Not a directory!";
      }
    }
    // Go to parent directory
    else if (choice == 'h') {
      // if "top level" reached, then show error.
      if (level == 0) 
        error = "No parent directory.";
      // otherwise go to parent directory (decreases level)
      else {
        selected_history[selector.path_] = {selected, print_start};
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
        if (selected_history.count(selector.path_) > 0) {
          selected = selected_history.at(selector.path_).first;
          print_start = selected_history.at(selector.path_).second;
        }
      }
    }
    // Move choice down
    else if (choice == 'j') {
      if (selected == print_max-1 && selector.options_.size() > max)
        print_start++;
      else 
        selected = utils::Mod(selected+1, visible_options.size());
    }
    // Move choice up
    else if (choice == 'k') {
      if (selected == 0 && print_start > 0)
        print_start--;
      else 
        selected = utils::Mod(selected-1, print_max);
    }
    // Select audio-track
    else if (std::to_string(choice) == "10") {
      std::filesystem::path select_path = visible_options[selected].first;
      if (select_path.filename().extension() == ".mp3" || select_path.filename().extension() == ".wav")
        break;
      else 
        error = "Wrong file type. Select mp3 or wav";
    }
    // Jump to top
    else if (choice == 'g') {
      selected = 0;
      print_start = 0;
    }
    // Add path to known music-libraries
    else if (choice == '+') {
      std::string input = InputString("Absolute path: ");
      if (std::filesystem::exists(input)) {
        if (input.back() == '/')
          input.pop_back();
        audio_paths_.push_back(input);
        nlohmann::json audio_paths = utils::LoadJsonFromDisc(base_path_ + MUSIC_PATHS);
        audio_paths.push_back(input);
        utils::WriteJsonToDisc(base_path_ + MUSIC_PATHS, audio_paths);
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
  utils::WriteJsonToDisc(base_path_ + RECENTLYPLAYER_PATH, j_recently_played);

  // Return selected path.
  return select_path.string(); 
}

void ClientGame::EditSettings() {
  drawrer_.ClearField();
  drawrer_.PrintCenteredLine(LINES/4, "SETTINGS");
  drawrer_.PrintCenteredLine(LINES/4+1, "use 'j'/'k' to cycle selection. Hit '[enter]' to toggle setting");
  auto settings = utils::LoadJsonFromDisc(base_path_ + SETTINGS_PATH);
  int cur_selection = 0;
  std::map<int, std::string> settings_mapping = {{0, "stay_in_synapse_menu"}, {1, "show_full_welcome"},
    {2, "music_on"}, {3, "show_ai_tactics"}};
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

    if (cur_selection == 3) attron(COLOR_PAIR(COLOR_AVAILIBLE));
    drawrer_.PrintCenteredLine(LINES/4+19, "Show AI Tactics before game-start.");
    attron(COLOR_PAIR(COLOR_DEFAULT));
    drawrer_.PrintCenteredLine(LINES/4+20, (settings["show_ai_tactics"]) ? "yes" : "no");

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
  utils::WriteJsonToDisc(base_path_ + SETTINGS_PATH, settings);
  drawrer_.ClearField();
  drawrer_.PrintCenteredLine(LINES/2, "Settings saved. Restarting game.");
  drawrer_.PrintCenteredLine(LINES/2+2, "[Press any key to close game]");
  getch();
  WrapUp();
}

void ClientGame::LoadSettings() {
  nlohmann::json settings = utils::LoadJsonFromDisc(base_path_ + SETTINGS_PATH);
  stay_in_synapse_menu_ = settings["stay_in_synapse_menu"];
  show_full_welcome_text_ = settings["show_full_welcome"];
  music_on_ = settings["music_on"];
  show_ai_tactics_ = settings["show_ai_tactics"];
}

void ClientGame::LoadRanking() {
  nlohmann::json ranking = utils::LoadJsonFromDisc(base_path_ + RANKING_PATH);
  spdlog::get(LOGGER)->debug("Got ranking: {}, size: {}", ranking.dump(), ranking.size());
  if (ranking.size() == 0)
    return;
  for (const auto& it : ranking.get<std::map<std::string, nlohmann::json>>()) {
    ranking_[it.first] = new RankingEntry(it.second);
  }
}

void ClientGame::StoreRanking() {
  nlohmann::json ranking;
  for (const auto& it : ranking_) {
    ranking[it.first] = it.second->ToJson();
    spdlog::get(LOGGER)->debug("StoreRanking: {}: {}", it.first, ranking[it.first].dump());
  }
  utils::WriteJsonToDisc(base_path_ + RANKING_PATH, ranking);
}

void ClientGame::PrintRanking() {
  spdlog::get(LOGGER)->debug("ClientGame::PrintRanking()");
  drawrer_.ClearField();
  drawrer_.PrintCenteredLineBold(5, "LOCAL RANKING");
  RankingEntry header("filename", {0,0}, 6, 7, 8);
  std::string header_str = header.ToString();
  auto pos = header_str.rfind("8");
  header_str.replace(pos, 5, "draw ");
  pos = header_str.rfind("7");
  header_str.replace(pos, 5, "lost ");
  pos = header_str.rfind("6");
  header_str.replace(pos, 5, "won  ");
  pos = header_str.rfind("0x0");
  header_str.replace(pos, 5, "size ");
  drawrer_.PrintCenteredLineBold(7, header_str);

  int counter=0;
  for (const auto& it : ranking_) {
    drawrer_.PrintCenteredLine(8+counter++, it.second->ToString());
  }
  drawrer_.PrintCenteredLine(15+counter, "[Press any key to quit]");
  refresh();
  getch();
  WrapUp();
}


void ClientGame::h_InitTutorialText(const texts::paragraphs_field_t& text) {
  Pause();
  // In case another tutorial-message is added use last-context of current
  // context, instead of current_context.
  int old_context = current_context_;
  if (current_context_ == CONTEXT_TEXT) 
    old_context = contexts_.at(CONTEXT_TEXT).last_context();
  std::this_thread::sleep_for(std::chrono::milliseconds(TUTORIAL_MSG_LAG));
  contexts_.at(CONTEXT_TEXT).init_text(text, old_context);
  current_context_ = CONTEXT_TEXT;
  h_TextPrint();
}

void ClientGame::h_TextQuit(std::shared_ptr<Data>) {
  h_TextQuit();
}

void ClientGame::h_TextQuit() {
  std::shared_lock sl(mutex_context_);
  // Get last context from context-text, to resume where we left off before text-context.
  current_context_ = contexts_.at(CONTEXT_TEXT).last_context();
  // Make sure this context exists (otherwise use default context (CONTEXT_RESOURCES).
  if (contexts_.count(current_context_) == 0)
    current_context_ = CONTEXT_RESOURCES;
  drawrer_.set_topline(contexts_.at(current_context_).topline());
  drawrer_.set_viewpoint(current_context_);
  UnPause();
}

void ClientGame::h_TextPrint() {
  auto paragraph = contexts_.at(current_context_).get_paragraph();
  drawrer_.PrintCenteredParagraphAndMiniFields(paragraph.first, paragraph.second, true);
}

void ClientGame::Pause() {
  pause_ = true;
  drawrer_.set_stop_render(true);
  ws_srv_->SendMessage("set_pause_on", std::make_shared<Data>());
  audio_.Pause();
}

void ClientGame::UnPause() {
  pause_ = false;
  ws_srv_->SendMessage("set_pause_off", std::make_shared<Data>());
  audio_.Unpause();
  drawrer_.set_stop_render(false);
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
