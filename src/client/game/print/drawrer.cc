#include "client/game/print/drawrer.h"
#include "curses.h"
#include "share/constants/codes.h"
#include "share/defines.h"
#include "share/objects/units.h"
#include "spdlog/spdlog.h"
#include <mutex>
#include <utility>

#define SIDE_COLUMN_WIDTH 30
#define BORDER_LINES 2 

Drawrer::Drawrer() {
  cur_view_point_ = VP_RESOURCE;
  cur_selection_ = {
    {VP_FIELD, {-1, -1, &ViewPoint::inc_field, &ViewPoint::to_string_field}},
    {VP_RESOURCE, {IRON, -1, &ViewPoint::inc_resource, &ViewPoint::to_string_resource}},
    {VP_TECH, {WAY, -1, &ViewPoint::inc_tech, &ViewPoint::to_string_tech}}
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
  else 
    markers_[type].clear();
}

void Drawrer::AddNewUnitToPos(position_t pos, int unit, int color) {
  std::unique_lock ul(mutex_print_field_);
  if (unit == UnitsTech::ACTIVATEDNEURON)
    field_[pos.first][pos.second] = {SYMBOL_DEF, color};
  else if (unit == UnitsTech::SYNAPSE)
    field_[pos.first][pos.second] = {SYMBOL_BARACK, color};
  else if (unit == UnitsTech::NUCLEUS)
    field_[pos.first][pos.second] = {SYMBOL_DEN, color};
  else if (unit == UnitsTech::RESOURCENEURON)
    field_[pos.first][pos.second] = {field_[pos.first][pos.second].symbol_, color};
}

void Drawrer::UpdateTranser(nlohmann::json &transfer_json) {
  std::unique_lock ul(mutex_print_field_);
  Transfer t(transfer_json);
  transfer_.set_resources(t.resources());
  transfer_.set_technologies(t.technologies());
  transfer_.set_audio_played(t.audio_played());
  transfer_.set_players(t.players());

  // Remove temp fields.
  for (const auto& it : temp_symbols_)
    field_[it.first.first][it.first.second] = it.second;
  temp_symbols_.clear();

  // Add potentials
  for (const auto& it : t.potentials()) {
    // Add potential to field as temporary.
    temp_symbols_[it.first] = field_[it.first.first][it.first.second];
    field_[it.first.first][it.first.second] = {it.second.first, it.second.second};
  }

  // Update dead neurons
  for (const auto & it : t.new_dead_neurons()) {
    ul.unlock();
    AddNewUnitToPos(it.first, it.second, COLOR_DEFAULT);
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

void Drawrer::PrintCenteredLine(int l, std::string line) {
  std::string clear_string(COLS, ' ');
  mvaddstr(l, 0, clear_string.c_str());
  mvaddstr(l, COLS/2-line.length()/2, line.c_str());
}

void Drawrer::PrintCenteredLineColored(int l, std::vector<std::pair<std::string, int>> txt_with_color) {
  // Get total length.
  unsigned int total_length = 0;
  for (const auto& it : txt_with_color) 
    total_length += it.first.length();

  // Print parts one by one and update color for each part.
  unsigned int position = COLS/2-total_length/2;
  for (const auto& it : txt_with_color) {
    attron(COLOR_PAIR(it.second));
    mvaddstr(l, position, it.first.c_str());
    position += it.first.length();
    attron(COLOR_PAIR(COLOR_DEFAULT));
  }
}

void Drawrer::PrintCenteredParagraph(texts::paragraph_t paragraph) {
    int size = paragraph.size()/2;
    int counter = 0;
    for (const auto& line : paragraph)
      PrintCenteredLine(LINES/2-size+(counter++), line);
}

void Drawrer::PrintCenteredParagraphs(texts::paragraphs_t paragraphs) {
  for (auto& paragraph : paragraphs) {
    ClearField();
    paragraph.push_back({});
    paragraph.push_back("[Press any key to continue]");
    PrintCenteredParagraph(paragraph);
    getch();
  }
}

void Drawrer::PrintGame(bool only_field, bool only_side_column) {
  std::unique_lock ul(mutex_print_field_);
  if (stop_render_ || !transfer_.initialized()) 
    return;
  PrintHeader(transfer_.audio_played(), transfer_.PlayersToString());
  if (!only_side_column)
    PrintField();
  if (!only_field)
    PrintSideColumn(transfer_.resources(), transfer_.technologies());
  PrintMessage();
  PrintFooter(cur_selection_.at(cur_view_point_).to_string(transfer_));
  refresh();
}

void Drawrer::PrintHeader(float audio_played, const std::string& players) {
  PrintCenteredLine(l_headline_, "DISSONANCE");
  PrintCenteredLine(l_headline_+1, players);

  // Print audio
  unsigned int length = (c_resources_ - c_field_ - 20) * audio_played;
  for (unsigned int i=0; i<length; i++)
    mvaddstr(l_audio_, c_field_+10+i, "x");
}

void Drawrer::PrintTopline(std::vector<std::pair<std::string, int>> topline) {
  std::unique_lock ul(mutex_print_field_);
  // Print topline
  ClearLine(l_bar_);
  PrintCenteredLineColored(l_bar_, topline);
}

void Drawrer::PrintField() {
  spdlog::get(LOGGER)->info("Drawrer::PrintField");
  position_t sel = field_pos();
  auto range = cur_selection_.at(VP_FIELD).range_;

  for (unsigned int l=0; l<field_.size(); l++) {
    for (unsigned int c=0; c<field_[l].size(); c++) {
      position_t cur = {l, c};
      // Select color 
      if (cur_view_point_ == VP_FIELD && cur == sel)
        attron(COLOR_PAIR(COLOR_AVAILIBLE));
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
      else 
        mvaddstr(l_main_+extra_height_+l, c_field_+extra_width_+2*c, field_[l][c].symbol_.c_str());
      
      // Add extra with space for map shape and change color back to default.
      mvaddch(l_main_+extra_height_+l, c_field_+extra_width_+ 2*c+1, ' ' );
      attron(COLOR_PAIR(COLOR_DEFAULT));
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
  mvaddstr(l_main_, c_resources_, "RESOURCES");
  int inc = (static_cast<int>(resources.size()*2 + technologies.size()*2+4) > field_height()) ? 1 : 2;
  unsigned int counter = 2;
  for (const auto& it : resources) {
    if (cur_selection_.at(VP_RESOURCE).x_ == it.first)
      attron(COLOR_PAIR(COLOR_AVAILIBLE));
    else if (it.second.active_)
      attron(COLOR_PAIR(COLOR_SUCCESS));
    std::string str = resource_symbol_mapping.at(it.first) + " " + resources_name_mapping.at(it.first) 
      + ": " + it.second.value_;
    ClearLine(l_main_ + counter, c_resources_);
    mvaddstr(l_main_ + counter, c_resources_, str.c_str());
    attron(COLOR_PAIR(COLOR_DEFAULT));
    counter += inc;
  }
  counter += 2;
  mvaddstr(l_main_ + counter, c_resources_, "TECHNOLOGIES");
  counter += 2;
  for (const auto& it : technologies) {
    if (cur_selection_.at(VP_TECH).x_ == it.first)
      attron(COLOR_PAIR(COLOR_AVAILIBLE));
    else if (it.second.active_)
      attron(COLOR_PAIR(COLOR_SUCCESS));
    std::string str = units_tech_name_mapping.at(it.first) + " (" + it.second.cur_ + "/" + it.second.max_ + ")";
    mvaddstr(l_main_ + counter, c_resources_, str.c_str());
    attron(COLOR_PAIR(COLOR_DEFAULT));
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

void Drawrer::ClearLine(int line, int start_col) {
  std::string clear_string(COLS - start_col, ' ');
  mvaddstr(line, start_col, clear_string.c_str());
}
