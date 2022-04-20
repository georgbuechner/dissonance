#include "client/game/print/drawrer.h"
#include "curses.h"
#include "drawrer.h"
#include "server/game/field/field.h"
#include "server/game/player/statistics.h"
#include "share/constants/codes.h"
#include "share/defines.h"
#include "share/objects/transfer.h"
#include "share/objects/units.h"
#include "share/tools/random/random.h"
#include "share/tools/utils/utils.h"
#include "spdlog/spdlog.h"
#include <mutex>
#include <string>
#include <utility>

#define SIDE_COLUMN_WIDTH 30
#define BORDER_LINES 2 

Drawrer::Drawrer() {
  cur_view_point_ = VP_RESOURCE;
  cur_selection_ = {
    {VP_FIELD, {-1, -1, &ViewPoint::inc_field, &ViewPoint::to_string_field}},
    {VP_RESOURCE, {IRON, -1, &ViewPoint::inc_resource, &ViewPoint::to_string_resource}},
    {VP_TECH, {WAY, -1, &ViewPoint::inc_tech, &ViewPoint::to_string_tech}},
    {VP_TECH, {WAY, -1, &ViewPoint::inc_tech, &ViewPoint::to_string_tech}},
    {VP_POST_GAME, {WAY, -1, &ViewPoint::inc_resource, &ViewPoint::to_string_tech}},
  };
  stop_render_ = false;
}

int Drawrer::field_height() {
  return l_message_ - l_main_ - 1;
}

int Drawrer::field_width() {
  return (c_resources_ - SIDE_COLUMN_WIDTH/2 - 3)/2;
}

position_t Drawrer::field_pos() {
  return {cur_selection_.at(VP_FIELD).y_, cur_selection_.at(VP_FIELD).x_};
}

std::string Drawrer::game_id_from_lobby() {
  std::unique_lock ul(mutex_print_field_);
  if (cur_view_point_ == VP_LOBBY && (unsigned int)cur_selection_.at(VP_LOBBY).x_ < lobby_.lobby().size())
    return lobby_.lobby()[cur_selection_.at(VP_LOBBY).x_]._game_id;
  return "";
}

int Drawrer::GetResource() {
  if (cur_view_point_ != VP_RESOURCE)
    return -1;
  return cur_selection_.at(VP_RESOURCE).x_;
}

int Drawrer::GetTech() {
  if (cur_view_point_ != VP_TECH)
    return -1;
  return cur_selection_.at(VP_TECH).x_;
}

void Drawrer::set_viewpoint(int viewpoint) {
  if (viewpoint <= 2)
    cur_view_point_ = viewpoint;
  else if (viewpoint == VP_POST_GAME) {
    cur_view_point_ = viewpoint;
    cur_selection_[VP_POST_GAME] = ViewPoint(0, statistics_.size(), &ViewPoint::inc_stats, &ViewPoint::to_string_tech);
  }
  else if (viewpoint == VP_LOBBY) {
    cur_view_point_ = VP_LOBBY;
    cur_selection_[VP_LOBBY] = ViewPoint(0, 0, &ViewPoint::inc_stats, &ViewPoint::to_string_tech);
  }
  spdlog::get(LOGGER)->info("Set current viewpoint to {}", cur_view_point_);
}

void Drawrer::inc_cur_sidebar_elem(int value) {
  spdlog::get(LOGGER)->debug("Drawrer::inc_cur_sidebar_elem: {}", value);
  cur_selection_.at(cur_view_point_).inc(value);
}

void Drawrer::set_field_start_pos(position_t pos) {
  cur_selection_.at(VP_FIELD).y_ = pos.first;
  cur_selection_.at(VP_FIELD).x_ = pos.second;
}

void Drawrer::set_range(std::pair<position_t, int> range) {
  cur_selection_.at(VP_FIELD).range_ = range;
}

void Drawrer::set_topline(t_topline topline) {
  topline_ = topline;
}

void Drawrer::set_mode(int mode) {
  mode_ = mode;
  if (mode_ == OBSERVER) {
    c_field_ += SIDE_COLUMN_WIDTH/3;
    c_resources_ += SIDE_COLUMN_WIDTH/3; 
  }
}

void Drawrer::set_statistics(nlohmann::json json) {
  spdlog::get(LOGGER)->info("Stet statistics: {}", json.dump());
  for (const auto& it : json.get<std::map<std::string, nlohmann::json>>())
    statistics_[it.first] = Statictics(it.second);
}

bool Drawrer::InGraph(position_t pos) {
  return graph_positions_.count(pos) > 0;
}
bool Drawrer::Free(position_t pos) {
  return field_[pos.first][pos.second].symbol_ == SYMBOL_FREE;
}

int Drawrer::next_viewpoint() {
  cur_view_point_ = (1+cur_view_point_+1)%2 + 1;
  return cur_view_point_;
}

void Drawrer::set_msg(std::string msg) {
  msg_ = msg;
}

void Drawrer::set_transfer(nlohmann::json& data) {
  transfer_ = Transfer(data);
  field_ = transfer_.field();

  // Adjust start-point of field if game size is smaller than resolution suggests.
  unsigned int cur_height = field_height();
  unsigned int cur_width = field_width();
  extra_height_ = (cur_height > field_.size()) ? (cur_height-field_.size())/2 : 0;
  extra_width_ = (cur_width > field_[0].size()) ? cur_width-field_[0].size() : 0;
  for (const auto& it : transfer_.graph_positions())
    graph_positions_.insert(it);
  cur_selection_.at(VP_FIELD).graph_positions_ = graph_positions_;
}

void Drawrer::set_stop_render(bool stop) {
  std::unique_lock ul(mutex_print_field_);
  stop_render_ = stop;
}

void Drawrer::AddMarker(int type, position_t pos, int color, std::string symbol) {
  std::unique_lock ul(mutex_print_field_);
  // If Symbol is not given, use current field symbol.
  if (symbol == "")
    markers_[type][pos] = {field_[pos.first][pos.second].symbol_, color};
  else 
    markers_[type][pos] = {symbol, color};
}

position_t Drawrer::GetMarkerPos(int type, std::string symbol) {
  std::unique_lock sl(mutex_print_field_);
  for (const auto& it : markers_[type]) {
    if (it.second.first == symbol)
      return it.first;
  }
  return {-1, -1};
}

void Drawrer::ClearMarkers(int type) {
  std::unique_lock ul(mutex_print_field_);
  if (type == -1)
    markers_.clear();
  else if (markers_.count(type) > 0)
    markers_[type].clear();
}

void Drawrer::AddNewUnitToPos(position_t pos, int unit, int color) {
  std::unique_lock ul(mutex_print_field_);
  if (unit == UnitsTech::RESOURCENEURON)
    field_[pos.first][pos.second] = {field_[pos.first][pos.second].symbol_, COLOR_RESOURCES};
  else if (unit_symbol_mapping.count(unit) > 0)
    field_[pos.first][pos.second] = {unit_symbol_mapping.at(unit), color};
}

void Drawrer::UpdateTranser(nlohmann::json &transfer_json) {
  std::unique_lock ul(mutex_print_field_);
  Transfer t(transfer_json);
  transfer_.set_resources(t.resources());
  transfer_.set_technologies(t.technologies());
  transfer_.set_audio_played(t.audio_played());
  transfer_.set_players(t.players());
  transfer_.set_build_options(t.build_options());
  transfer_.set_synapse_options(t.synapse_options());

  // Remove temp fields.
  for (const auto& it : temp_symbols_)
    field_[it.first.first][it.first.second] = it.second;
  temp_symbols_.clear();
  
  // Update dead neurons
  ul.unlock();
  for (const auto & it : t.new_dead_neurons())
    AddNewUnitToPos(it.first, it.second, COLOR_DEFAULT);

  // Add potentials
  for (const auto& it : t.potentials()) {
    // Add potential to field as temporary.
    temp_symbols_[it.first] = field_[it.first.first][it.first.second];
    field_[it.first.first][it.first.second] = {it.second.first, it.second.second};
  }
}

void Drawrer::UpdateLobby(nlohmann::json& lobby_json, bool init) {
  std::unique_lock ul(mutex_print_field_);
  if (lobby_json.contains("lobby") && !lobby_json["lobby"].is_null()) {
    lobby_ = Lobby(lobby_json["lobby"]);
    cur_selection_.at(VP_LOBBY).y_ = lobby_.lobby().size();
  }
  else {
    lobby_ = Lobby();
    cur_selection_.at(VP_LOBBY).y_ = 0;
    cur_selection_.at(VP_LOBBY).x_ = 0;
  }
}

void Drawrer::SetUpBorders(int lines, int cols) {
  spdlog::get(LOGGER)->info("Drawrer::SetUpBorders: {}={}, {}={}", lines, LINES, cols, COLS);
  // Setup lines 
  l_headline_ = 1; 
  l_audio_ = 4; 
  l_bar_ = 6;
  l_main_ = 8; // implies complete lines for heading = 8
  int lines_field = lines - l_main_ - BORDER_LINES; 
  l_message_ = lines_field + 1;
  l_bottom_ = l_message_ + BORDER_LINES ;  

  // Setup cols
  c_field_ = SIDE_COLUMN_WIDTH/2;
  c_resources_ = cols - SIDE_COLUMN_WIDTH - c_field_ + 3;
}

void Drawrer::ClearField() {
  clear();
  refresh();
}

void Drawrer::PrintCenteredLine(int l, std::string line) const {
  std::string clear_string(COLS, ' ');
  mvaddstr(l, 0, clear_string.c_str());
  mvaddstr(l, COLS/2-line.length()/2, line.c_str());
}
void Drawrer::PrintCenteredLineBold(int l, std::string line) const {
  std::string clear_string(COLS, ' ');
  attron(WA_BOLD);
  mvaddstr(l, 0, clear_string.c_str());
  mvaddstr(l, COLS/2-line.length()/2, line.c_str());
  attroff(WA_BOLD);
}

void Drawrer::PrintCenteredLineColored(int l, std::vector<std::pair<std::string, int>> txt_with_color) {
  // Get total length.
  unsigned int total_length = 0;
  for (const auto& it : txt_with_color) 
    total_length += it.first.length();

  // Print parts one by one and update color for each part.
  unsigned int position = COLS/2-total_length/2;
  for (const auto& it : txt_with_color) {
    if (it.second == COLOR_AVAILIBLE)
      attron(WA_BOLD);
    attron(COLOR_PAIR(it.second));
    mvaddstr(l, position, it.first.c_str());
    position += it.first.length();
    attron(COLOR_PAIR(COLOR_DEFAULT));
    attroff(WA_BOLD);
  }
}

void Drawrer::PrintCenteredParagraph(texts::paragraph_t paragraph, bool nextmsg) {
  if (nextmsg) {
    paragraph.push_back("");
    paragraph.push_back("[press 'h' for previous and 'l' for next text. 'q' to quit.]");
  }
  PrintCenteredParagraph(paragraph, (unsigned int)0);
}
void Drawrer::PrintCenteredParagraph(texts::paragraph_t paragraph, unsigned int additional_height) {
  int counter = 0;
  unsigned int start_height = LINES/2 - paragraph.size()/2 - additional_height;
  for (const auto& line : paragraph)
    PrintCenteredLine(start_height+counter++, line);
}

void Drawrer::PrintCenteredParagraphAndMiniFields(texts::paragraph_t paragraph, std::vector<std::string> fields, 
    bool nextmsg) {
  std::string fields_str = "";
  for (const auto& field : fields) 
    fields_str += field + ", ";
  spdlog::get(LOGGER)->debug("Printing paragraph with fields: {}", fields_str);
  PrintCenteredParagraph(paragraph, (unsigned int)5);

  unsigned int start_height = LINES/2 + paragraph.size()/2 + 2;

  for (unsigned int r=0; r<10; r++) {
    unsigned int start_col = COLS/2-10*fields.size() - 5*(fields.size()/2);
    for (unsigned int field=0; field<fields.size(); field++) {
      for (unsigned int c=0; c<10; c++) {
        auto sym = mini_fields_.at(fields[field])[r][c];
        attron(COLOR_PAIR(sym.color_));
        mvaddstr(start_height+r, start_col++, sym.symbol_.c_str());
        mvaddch(start_height+r, start_col++, ' ');
        attron(COLOR_PAIR(COLOR_DEFAULT));
      }
      start_col += 5;
    }
  }
  spdlog::get(LOGGER)->debug("done");
}

void Drawrer::PrintCenteredParagraphs(texts::paragraphs_t paragraphs, bool skip_first_wait) {
  // int counter = 0;
  for (auto& paragraph : paragraphs) {
    ClearField();
    paragraph.push_back({});
    paragraph.push_back("[Press any key to continue]");
    PrintCenteredParagraph(paragraph);
    getch();
  }
}

void Drawrer::PrintGame(bool only_field, bool only_side_column, int context) {
  std::unique_lock ul(mutex_print_field_);
  if (stop_render_ || !transfer_.initialized()) 
    return;
  // Print headline
  PrintHeader(transfer_.audio_played(), transfer_.PlayersToPrint());
  // Print topline.
  std::vector<bool> topline_colors; 
  if (context == CONTEXT_RESOURCES || context == CONTEXT_TECHNOLOGIES) 
    topline_colors = transfer_.build_options();
  else if (context == CONTEXT_SYNAPSE) 
    topline_colors = transfer_.synapse_options();

  if (mode_ != OBSERVER)
    PrintTopline(topline_colors);

  // Print field and/or side-column
  if (!only_side_column)
    PrintField();
  if (!only_field && mode_ != OBSERVER)
    PrintSideColumn(transfer_.resources(), transfer_.technologies());

  // Print footer and message
  if (mode_ != OBSERVER) {
    PrintMessage();
    PrintFooter(cur_selection_.at(cur_view_point_).to_string(transfer_));
  }
  refresh();
}

void Drawrer::PrintHeader(float audio_played, const t_topline& players) {
  PrintCenteredLineBold(l_headline_, "DISSONANCE");
  ClearLine(l_headline_+1);
  PrintCenteredLineColored(l_headline_+1, players);
  // Print audio
  unsigned int length = (c_resources_ - c_field_ - 20) * audio_played;
  for (unsigned int i=0; i<length; i++)
    mvaddstr(l_audio_, c_field_+10+i, "x");
}

void Drawrer::PrintTopline(std::vector<bool> availible_options) {
  // Update copied topline and mark availible-options with COLOR_AVAILIBLE.
  t_topline copied_topline = topline_;
  for (unsigned int i=0; i<copied_topline.size(); i++) {
    if (availible_options.size() > i && availible_options[i])
      copied_topline[i].second = COLOR_AVAILIBLE;
  }
  // Print topline.
  ClearLine(l_bar_);
  PrintCenteredLineColored(l_bar_, copied_topline);
}

void Drawrer::PrintField() {
  spdlog::get(LOGGER)->info("Drawrer::PrintField");
  position_t sel = field_pos();
  auto range = cur_selection_.at(VP_FIELD).range_;

  for (unsigned int l=0; l<field_.size(); l++) {
    for (unsigned int c=0; c<field_[l].size(); c++) {
      position_t cur = {l, c};
      // If field-view, mark when selected.
      if (cur_view_point_ == VP_FIELD && cur == sel) {
        attron(WA_BOLD);
        attron(COLOR_PAIR(COLOR_AVAILIBLE));
      }
      // If field-view, mark when availible.
      else if (cur_view_point_ == VP_FIELD && utils::Dist(cur, range.first) <= range.second && 
          graph_positions_.count(cur) > 0)
        attron(COLOR_PAIR(COLOR_SUCCESS));
      else {
        int color = field_[l][c].color_;
        if (!can_change_color() && color >= 10)
          color = (color % 2) + 1;
        attron(COLOR_PAIR(color));
      }
      // Print symbol or marker
      auto replacment = GetReplaceMentFromMarker(cur);
      if (replacment.first != "") {
        attron(COLOR_PAIR(replacment.second));
        mvaddstr(l_main_+extra_height_+l, c_field_+extra_width_+2*c, replacment.first.c_str());
      }
      else if (cur_view_point_ == VP_FIELD && cur == sel)
        mvaddstr(l_main_+extra_height_+l, c_field_+extra_width_+2*c, "x");
      else 
        mvaddstr(l_main_+extra_height_+l, c_field_+extra_width_+2*c, field_[l][c].symbol_.c_str());
      // Add extra with space for map shape and change color back to default.
      mvaddch(l_main_+extra_height_+l, c_field_+extra_width_+ 2*c+1, ' ' );
      attron(COLOR_PAIR(COLOR_DEFAULT));
      attroff(WA_BOLD);
    }
  }
}

std::pair<std::string, int> Drawrer::GetReplaceMentFromMarker(position_t pos) {
  std::pair<std::string, int> symbol = {"", COLOR_DEFAULT};
  for (const auto& it : markers_) {
    if (it.second.count(pos) > 0)
      symbol = it.second.at(pos);
  }
  return symbol;
}

void Drawrer::PrintSideColumn(const std::map<int, Transfer::Resource>& resources,
    const std::map<int, Transfer::Technology>& technologies) {
  attron(WA_BOLD);
  mvaddstr(l_main_, c_resources_, "RESOURCES");
  attroff(WA_BOLD);
  int inc = (static_cast<int>(resources.size()*2 + technologies.size()*2+4) > field_height()) ? 1 : 2;
  unsigned int counter = 2;
  for (const auto& it : resources) {
    // Mark selected resource/ technology
    if (cur_selection_.at(VP_RESOURCE).x_ == it.first) {
      attron(WA_BOLD);
      attron(COLOR_PAIR(COLOR_AVAILIBLE));
    }
    // Mark active resouce/ technology
    else if (it.second.active_)
      attron(COLOR_PAIR(COLOR_SUCCESS));
    // Get string.
    std::string str = resource_symbol_mapping.at(it.first) + " " + resources_name_mapping.at(it.first) 
      + ": " + it.second.value_;
    // Clear line before printing, then print
    ClearLine(l_main_ + counter, c_resources_);
    mvaddstr(l_main_ + counter, c_resources_, str.c_str());
    attron(COLOR_PAIR(COLOR_DEFAULT));
    attroff(WA_BOLD);
    counter += inc;
  }
  counter += 2;
  attron(WA_BOLD);
  mvaddstr(l_main_ + counter, c_resources_, "TECHNOLOGIES");
  attroff(WA_BOLD);
  counter += 2;
  for (const auto& it : technologies) {
    if (cur_selection_.at(VP_TECH).x_ == it.first) {
      attron(WA_BOLD);
      attron(COLOR_PAIR(COLOR_AVAILIBLE));
    }
    else if (it.second.active_)
      attron(COLOR_PAIR(COLOR_SUCCESS));
    std::string str = units_tech_name_mapping.at(it.first) + " (" + it.second.cur_ + "/" + it.second.max_ + ")";
    mvaddstr(l_main_ + counter, c_resources_, str.c_str());
    attron(COLOR_PAIR(COLOR_DEFAULT));
    attroff(WA_BOLD);
    counter += inc;
  }
}

void Drawrer::PrintMessage() {
  ClearLine(l_message_);
  mvaddstr(l_message_, c_field_, msg_.c_str());
}

void Drawrer::PrintFooter(std::string str) {
  auto lines = utils::Split(str, "$");
  for (unsigned int i=0; i<lines.size(); i++) {
    ClearLine(l_bottom_ + i);
    PrintCenteredLine(l_bottom_ + i, lines[i]);
  }
}

void Drawrer::PrintStatistics() const {
  spdlog::get(LOGGER)->info("Printing statistics: {} {}", statistics_.size(), cur_selection_.at(VP_POST_GAME).x_);
  clear();
  refresh();
  int start_line = LINES/8;
  int counter = 0;
  for (const auto& it : statistics_) {
    if (counter++ == cur_selection_.at(VP_POST_GAME).x_) {
      // Print player name and "headings" bold. Print only player name in player-color.
      attron(COLOR_PAIR(it.second.player_color()));
      // Player name.
      PrintCenteredLineBold(start_line, utils::ToUpper(it.first));
      attroff(COLOR_PAIR(it.second.player_color()));
      int i=2;
      PrintCenteredLineBold(start_line+(++i), "Neurons Built");
      for (const auto& it : it.second.neurons_build()) 
        PrintCenteredLine(start_line+(++i), units_tech_name_mapping.at(it.first) + ": " + std::to_string(it.second));
      i++;
      PrintCenteredLineBold(start_line+(++i), "Potentials Built");
      for (const auto& it : it.second.potentials_build()) 
        PrintCenteredLine(start_line+(++i), units_tech_name_mapping.at(it.first) + ": " + std::to_string(it.second));
      i++;
      PrintCenteredLineBold(start_line+(++i), "Potentials Killed");
      for (const auto& it : it.second.potentials_killed()) 
        PrintCenteredLine(start_line+(++i), units_tech_name_mapping.at(it.first) + ": " + std::to_string(it.second));
      i++;
      PrintCenteredLineBold(start_line+(++i), "Potentials Lost");
      for (const auto& it : it.second.potentials_lost()) 
        PrintCenteredLine(start_line+(++i), units_tech_name_mapping.at(it.first) + ": " + std::to_string(it.second));
      i++;
      PrintCenteredLineBold(start_line+(++i), "Enemy Epsps Swallowed By Ipsp");
      PrintCenteredLine(start_line+(++i), std::to_string(it.second.epsp_swallowed()));
      i++;
      PrintCenteredLineBold(start_line+(++i), "Resources");
      for (const auto& it : it.second.resources()) {
        PrintCenteredLine(start_line+(++i), resources_name_mapping.at(it.first));
        std::string info = "";
        for (const auto& jt : it.second)
          info += jt.first + ": " + utils::Dtos(jt.second) + ", ";
        info.substr(0, info.length()-2);
        PrintCenteredLine(start_line+(++i), info);
      }
      i++;
      PrintCenteredLineBold(start_line+(++i), "Technologies");
      for (const auto& it : it.second.technologies()) {
        std::string info = units_tech_name_mapping.at(it.first) + ": " 
          + utils::PositionToString(it.second);
        PrintCenteredLine(start_line+(++i), info);
      }
      PrintCenteredLine(start_line+2+(++i), "(press 'q' to quit.)");
    }
  }
}

void Drawrer::ClearLine(int line, int start_col) {
  std::string clear_string(COLS - start_col, ' ');
  mvaddstr(line, start_col, clear_string.c_str());
}

void Drawrer::PrintLobby() {
  std::unique_lock ul(mutex_print_field_);
  ClearField();
  PrintCenteredLine(LINES/4, "LOBBY");
  PrintCenteredLine(LINES/4+1, "use 'j'/'k' to cycle selection. Hit '[enter]' to join game.");
  PrintCenteredLine(LINES/4+2, msg_);
  // Check if lobby is empty
  if (lobby_.lobby().size() == 0) {
    PrintCenteredLine(LINES/4+5, "lobby is empty!");
  }
  // Otherwise print lobby
  else {
    unsigned int counter = 0;
    for (const auto& it : lobby_.lobby()) {
      std::string lobby_line = it._audio_map_name + ": " + std::to_string(it._cur_players) + "/" 
        + std::to_string(it._max_players);
      if ((unsigned int)cur_selection_.at(VP_LOBBY).x_ == counter/2)
        attron(COLOR_PAIR(COLOR_AVAILIBLE));
      PrintCenteredLine(LINES/4+5+counter, lobby_line);
      attron(COLOR_PAIR(COLOR_DEFAULT));
      counter += 2;
    }
  }
  refresh();
}

void Drawrer::CreateMiniFields(int player_color) {
  int enemy_color = (player_color == COLOR_P2) ? COLOR_P3 : COLOR_P2;

  spdlog::get(LOGGER)->info("Creating mini-fields");
  // Field test
  RandomGenerator* ran_gen = new RandomGenerator();
  Field* field = new Field(10, 10, ran_gen);
  field->BuildGraph();
  auto nucleus_test = field->AddNucleus(1);
  Player* p = new Player(nucleus_test.front(), field, ran_gen, player_color);
  mini_fields_["test"] = field->Export({p});
  delete field;

  // Simple map with nothing but one player-nucleus
  spdlog::get(LOGGER)->info("Simple map with nothing but one player-nucleus");
  field = new Field(10, 10, ran_gen);
  field->BuildGraph();
  auto nucleus = field->AddNucleus(1);
  auto exported_field = field->Export({p});
  exported_field[nucleus.front().first][nucleus.front().second].color_ = player_color;
  exported_field[nucleus_test.front().first][nucleus_test.front().second].color_ = COLOR_DEFAULT;
  mini_fields_["field_only_den"] = exported_field;
  delete field;
  delete p;

  // Simple map with player nucleus and resources (not activated)
  spdlog::get(LOGGER)->info("Simple map with player nucleus and resources (not activated)");
  field = new Field(10, 10, ran_gen);
  field->BuildGraph();
  nucleus = field->AddNucleus(1);

  spdlog::get(LOGGER)->debug("Creating new player");
  p = new Player(nucleus.front(), field, ran_gen, player_color);
  // Give player plenty resources (iron):
  for (unsigned int i=0; i<10; i++) 
    p->IncreaseResources(true);
  spdlog::get(LOGGER)->info("done");
  mini_fields_["field_player"] = field->Export({p});

  // Simple map with player nucleus and resources (oxygen activated)
  spdlog::get(LOGGER)->debug("Simple map with player nucleus and resources (oxygen activated)");
  p->DistributeIron(OXYGEN);
  p->DistributeIron(OXYGEN);
  mini_fields_["field_player_oxygen_activated"] = field->Export({p});

  // Simple map with player nucleus and resources (oxygen, potassium and glutamate activated)
  spdlog::get(LOGGER)->debug("Simple map with player nucleus and resources (oxygen, potassium and glutamate activated)");
  p->DistributeIron(POTASSIUM);
  p->DistributeIron(POTASSIUM);
  p->DistributeIron(GLUTAMATE);
  p->DistributeIron(GLUTAMATE);
  // Give player plenty resources (iron):
  for (unsigned int i=0; i<100; i++) 
    p->IncreaseResources(true);
  exported_field = field->Export({p});
  mini_fields_["field_player_oxygen_potassium_glutamat_activated"] = exported_field;

  // Simple map showing how to select position on map.
  spdlog::get(LOGGER)->debug("Simple map with showing highlighted range.");
  auto positions = field->GetAllInRange(nucleus.front(), 4, 0);
  for (const auto& pos : positions) {
    spdlog::get(LOGGER)->debug("pos: {}", utils::PositionToString(pos));
    // make sure it's in range
    if ((unsigned int)pos.first < exported_field.size() && (unsigned int)pos.second < exported_field[pos.first].size())
      exported_field[pos.first][pos.second].color_ = COLOR_SUCCESS;
  }
  spdlog::get(LOGGER)->debug("done highliting positions");
  exported_field[nucleus.front().first][nucleus.front().second].color_ = COLOR_AVAILIBLE;
  exported_field[nucleus.front().first][nucleus.front().second].symbol_ = "x";
  mini_fields_["field_select_neuron_position_1"] = exported_field;
  spdlog::get(LOGGER)->debug("done creating highliting-map");

  // Move selection on map.
  exported_field[nucleus.front().first][nucleus.front().second].color_ = player_color;
  exported_field[nucleus.front().first][nucleus.front().second].symbol_ = SYMBOL_DEN;
  auto pos = field->GetAllInRange(nucleus.front(), 2, 1.5, true).front();
  exported_field[pos.first][pos.second].color_ = COLOR_AVAILIBLE;
  exported_field[pos.first][pos.second].symbol_ = "x";
  mini_fields_["field_select_neuron_position_2"] = exported_field;

  // Add activated neuron.
  positions = field->GetAllInRange(nucleus.front(), 3, 2, true);
  exported_field[pos.first][pos.second].color_ = COLOR_SUCCESS; // mark position as free
  exported_field[pos.first][pos.second].symbol_ = SYMBOL_FREE; // mark position as free
  exported_field[positions.front().first][positions.front().second].color_ = COLOR_AVAILIBLE;
  exported_field[positions.front().first][positions.front().second].symbol_ = SYMBOL_DEF;
  mini_fields_["field_select_neuron_position_3"] = exported_field;
  
  // Simple map with player nucleus and resources (not activated)
  spdlog::get(LOGGER)->info("Enemy map with advancing potentials");
  Field* field_enemy = new Field(10, 10, ran_gen);
  field_enemy->BuildGraph();
  auto nucleus_enemy = field_enemy->AddNucleus(1);

  auto nucleus_pos_enemy = nucleus_enemy.front();
  spdlog::get(LOGGER)->debug("Creating new player");
  Player* p_enemy = new Player(nucleus_pos_enemy, field_enemy, ran_gen, enemy_color);
  // Give enemy plenty resources.
  for (unsigned int i=0; i<10; i++) 
    p_enemy->IncreaseResources(true);
  p_enemy->DistributeIron(OXYGEN);
  p_enemy->DistributeIron(OXYGEN);
  p_enemy->DistributeIron(POTASSIUM);
  p_enemy->DistributeIron(POTASSIUM);
  for (unsigned int i=0; i<100; i++) 
    p_enemy->IncreaseResources(true);
  auto exported_field_enemy = field->Export({p_enemy});
  // Add synape
  pos = field_enemy->GetAllInRange(nucleus_pos_enemy, 2, 1, true).front();
  exported_field_enemy[pos.first][pos.second].color_ = enemy_color;
  exported_field_enemy[pos.first][pos.second].symbol_ = SYMBOL_BARACK;
  // Add some epsp on enemy's way.
  auto way = field_enemy->GetWayForSoldier(nucleus_pos_enemy, {{9, 9}});
  std::vector<position_t> way_vec(way.begin(), way.end());
  for (unsigned int i=0; i<way_vec.size(); i+=3) {
    exported_field_enemy[way_vec[i].first][way_vec[i].second].color_ = enemy_color;
    exported_field_enemy[way_vec[i].first][way_vec[i].second].symbol_ = utils::CharToString('a', 
        ran_gen->RandomInt(1, 3));
  }
  mini_fields_["field_attack"] = exported_field_enemy;

  // Simple map showing enemy's epsp reaching your nucleus.
  way = field_enemy->GetWayForSoldier(nucleus.front(), {{9, 9}});
  std::vector<position_t> way_vec_2(way.begin(), way.end());
  for (unsigned int i=1; i<way_vec_2.size(); i+=2) {
    exported_field[way_vec_2[i].first][way_vec_2[i].second].color_ = enemy_color;
    exported_field[way_vec_2[i].first][way_vec_2[i].second].symbol_ = utils::CharToString('a', 
        ran_gen->RandomInt(1, 3));
  }
  mini_fields_["field_attack_2"] = exported_field;
  
  // Map showing how to select between multiple synapses.
  exported_field = mini_fields_["field_player_oxygen_potassium_glutamat_activated"];
  auto synapse_a = field->GetAllInRange(nucleus.front(), 2, 1.5, true).front();
  auto synapse_b = field->GetAllInRange(nucleus.front(), 3, 2, true).front();
  exported_field[synapse_a.first][synapse_a.second].color_ = player_color;
  exported_field[synapse_a.first][synapse_a.second].symbol_ = SYMBOL_BARACK;
  exported_field[synapse_b.first][synapse_b.second].color_ = player_color;
  exported_field[synapse_b.first][synapse_b.second].symbol_ = SYMBOL_BARACK;
  mini_fields_["multi_synapse_selection_1"] = exported_field;

  exported_field[synapse_a.first][synapse_a.second].color_ = COLOR_AVAILIBLE;
  exported_field[synapse_a.first][synapse_a.second].symbol_ = "a";
  exported_field[synapse_b.first][synapse_b.second].color_ = COLOR_AVAILIBLE;
  exported_field[synapse_b.first][synapse_b.second].symbol_ = "b";
  mini_fields_["multi_synapse_selection_2"] = exported_field;

  exported_field[synapse_a.first][synapse_a.second].color_ = COLOR_MARKED;
  exported_field[synapse_a.first][synapse_a.second].symbol_ = SYMBOL_BARACK;
  exported_field[synapse_b.first][synapse_b.second].color_ = player_color;
  exported_field[synapse_b.first][synapse_b.second].symbol_ = SYMBOL_BARACK;
  mini_fields_["multi_synapse_selection_3"] = exported_field;

  delete field;
  delete p;
  delete field_enemy;
  delete p_enemy;
  delete ran_gen;
}
