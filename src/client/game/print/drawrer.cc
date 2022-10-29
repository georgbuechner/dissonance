#include <algorithm>
#include <string>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>
#include "nlohmann/json.hpp"

#include "client/game/print/drawrer.h"
#include "server/game/field/field.h"
#include "share/objects/units.h"
#include "share/tools/random/random.h"
#include "share/tools/utils/utils.h"

#define SIDE_COLUMN_WIDTH 30
#define BORDER_LINES 2 

Drawrer::Drawrer() {
  cur_view_point_ = VP_RESOURCE;
  cur_selection_ = {
    {VP_FIELD, ViewPoint(-1, -1, &ViewPoint::inc_field, &ViewPoint::to_string_field)},
    {VP_RESOURCE, ViewPoint(IRON, -1, &ViewPoint::inc_resource, &ViewPoint::to_string_resource)},
    {VP_TECH, ViewPoint(WAY, -1, &ViewPoint::inc_tech, &ViewPoint::to_string_tech)},
    {VP_TECH, ViewPoint(WAY, -1, &ViewPoint::inc_tech, &ViewPoint::to_string_tech)},
    {VP_POST_GAME, ViewPoint(WAY, -1, &ViewPoint::inc_resource, &ViewPoint::to_string_tech)},
  };
  graph_positions_ = std::make_shared<std::set<position_t>>();
  stop_render_ = false;
  show_graph_ = false;
  show_resources_ = {{OXYGEN, true}, {POTASSIUM, true}, {CHLORIDE, false}, {GLUTAMATE, true}, 
    {DOPAMINE, false}, {SEROTONIN, false}};
}

int Drawrer::field_height() {
  return l_message_ - l_main_ - 1;
}

int Drawrer::field_width() {
  return (c_resources_ - SIDE_COLUMN_WIDTH/2 - 3)/2;
}

position_t Drawrer::field_pos() {
  return {cur_selection_.at(VP_FIELD).y(), cur_selection_.at(VP_FIELD).x()};
}

std::string Drawrer::game_id_from_lobby() {
  std::unique_lock ul(mutex_print_field_);
  if (cur_view_point_ == VP_LOBBY && (size_t)cur_selection_.at(VP_LOBBY).x() < lobby_->lobby().size())
    return lobby_->lobby()[cur_selection_.at(VP_LOBBY).x()]._game_id;
  return "";
}

std::vector<bool> Drawrer::synapse_options() {
  return update_->synapse_options();
}

int Drawrer::GetResource() {
  if (cur_view_point_ != VP_RESOURCE)
    return -1;
  return cur_selection_.at(VP_RESOURCE).x();
}

int Drawrer::GetTech() {
  if (cur_view_point_ != VP_TECH)
    return -1;
  return cur_selection_.at(VP_TECH).x();
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
}

void Drawrer::inc_cur_sidebar_elem(int value) {
  cur_selection_.at(cur_view_point_).inc(value);
}

void Drawrer::set_field_start_pos(position_t pos) {
  cur_selection_.at(VP_FIELD).set_y(pos.first);
  cur_selection_.at(VP_FIELD).set_x(pos.second);
}

void Drawrer::set_range(std::pair<position_t, int> range) {
  cur_selection_.at(VP_FIELD).set_range(range);
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

void Drawrer::set_statistics(std::vector<std::shared_ptr<Statictics>> statistics) {
  statistics_ = statistics;
}

bool Drawrer::InGraph(position_t pos) {
  return graph_positions_->count(pos) > 0;
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

void Drawrer::set_transfer(std::shared_ptr<Data> init, bool show_ai_tactics) {
  spdlog::get(LOGGER)->info("Drawrer::set_transfer.");
  field_ = init->field();

  // Adjust start-point of field if game size is smaller than resolution suggests.
  int cur_height = field_height();
  int cur_width = field_width();
  extra_height_ = ((size_t)cur_height > field_.size()) ? (cur_height-field_.size())/2 : 0;
  extra_width_ = ((size_t)cur_width > field_[0].size()) ? cur_width-field_[0].size() : 0;
  for (const auto& it : init->graph_positions())
    graph_positions_->insert(it);
  cur_selection_.at(VP_FIELD).set_graph_positions(graph_positions_);
  
  // set update
  spdlog::get(LOGGER)->debug("Drawrer::set_transfer: setting update.");
  update_ = std::dynamic_pointer_cast<Update>(init->update());
  // set resources and pointer to resources of resouce-viewpoint
  resources_ = std::make_shared<std::map<int, Data::Resource>>(init->update()->resources());
  cur_selection_.at(VP_RESOURCE).set_resources(resources_);
  // set technologies and pointer to technologies of tech-viewpoint
  technologies_ = std::make_shared<std::map<int, tech_of_t>>(init->technologies());
  cur_selection_.at(VP_TECH).set_technologies(technologies_);

  initialized_ = true;

  // Show player which macro they play with
  if (mode_ == SINGLE_PLAYER || mode_ == TUTORIAL) {
    std::string macro = (init->macro() == 0) ? "chained-potential" : "loophols";
    msg_ = "You are playing with \"" + macro + "\" as your macro!";
    if (show_ai_tactics) {
      set_stop_render(true);
      ClearField();
      PrintCenteredLine(LINES/2, "AI strategies");
      // Print ai strategies
      int counter=2;
      for (const auto& it : init->ai_strategies()) {
        if (it.second == 0xFFF)
          PrintCenteredLine(LINES/2+counter++, it.first);
        else 
          PrintCenteredLine(LINES/2+counter++, it.first + tactics_mapping.at(it.second));
      }
      PrintCenteredLine(LINES/2+counter+1, "[Press any key to continue]");
      std::unique_lock ul(mutex_print_field_);
      getch();
      ul.unlock();
      set_stop_render(false);
      ul.lock();
    }
  }
  spdlog::get(LOGGER)->info("Drawrer::set_transfer: done");
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

void Drawrer::ToggleGraphView() {
  show_graph_ = !show_graph_;
}

void Drawrer::ToggleShowResource(int resource) {
  if (show_resources_.count(resource) > 0)
    show_resources_[resource] = !show_resources_.at(resource);
}

void Drawrer::AddNewUnitToPos(position_t pos, int unit, int color) {
  spdlog::get(LOGGER)->debug("Drawrer::AddNewUnitToPos: pos: {}, unit: {}, color: {}", 
      utils::PositionToString(pos), unit, color);
  std::unique_lock ul(mutex_print_field_);
  // If resource-neuron deactivated, set color to default
  if (unit == UnitsTech::RESOURCENEURON && color == COLOR_DEFAULT)
    field_[pos.first][pos.second] = Data::Symbol({field_[pos.first][pos.second].symbol_, COLOR_DEFAULT});
  // If resource-neuron activated, set color to resource-color
  else if (unit == UnitsTech::RESOURCENEURON)
    field_[pos.first][pos.second] = Data::Symbol({field_[pos.first][pos.second].symbol_, COLOR_RESOURCES});
  // If loophole destroyed, set symbol to free.
  else if (unit == UnitsTech::LOOPHOLE && color == COLOR_DEFAULT)
    field_[pos.first][pos.second] = Data::Symbol({SYMBOL_FREE, COLOR_DEFAULT});
  // Otherwise add neuron with player-color.
  else if (unit_symbol_mapping.count(unit) > 0)
    field_[pos.first][pos.second] = Data::Symbol({unit_symbol_mapping.at(unit), color});
}

void Drawrer::AddTechnology(int technology) {
  if (technologies_->count(technology) && technologies_->at(technology).first < technologies_->at(technology).second) 
    technologies_->at(technology).first++;
}

void Drawrer::UpdateTranser(std::shared_ptr<Data> update) {
  spdlog::get(LOGGER)->debug("Drawrer::UpdateTranser");
  std::unique_lock ul(mutex_print_field_);
  *resources_ = update_->resources();
  update_ = std::dynamic_pointer_cast<Update>(update);

  // Remove temp fields.
  for (const auto& it : temp_symbols_)
    field_[it.first.first][it.first.second] = it.second;
  temp_symbols_.clear();
  
  // Update dead neurons
  ul.unlock();
  for (const auto & it : update->new_dead_neurons())
    AddNewUnitToPos(it.first, it.second, COLOR_DEFAULT);

  // Add potentials (pos: {symbol (1..9, a..z), color})
  for (const auto& it : update->potentials()) {
    // Add potential to field as temporary.
    temp_symbols_[it.first] = field_[it.first.first][it.first.second];
    int color = (update->hit_potentials().count(it.first) > 0) ? COLOR_MAGENTA : it.second.second;
    field_[it.first.first][it.first.second] = Data::Symbol({it.second.first, color});
  }
  spdlog::get(LOGGER)->debug("Drawrer::UpdateTranser: done");
}

void Drawrer::UpdateLobby(std::shared_ptr<Data> lobby_json, bool init) {
  std::unique_lock ul(mutex_print_field_);
  if (lobby_json->lobby().size() > 0) {
    lobby_ = std::dynamic_pointer_cast<Lobby>(lobby_json);
    cur_selection_.at(VP_LOBBY).set_y(lobby_->lobby().size());
  }
  else {
    lobby_ = std::make_shared<Lobby>();
    cur_selection_.at(VP_LOBBY).set_y(0);
    cur_selection_.at(VP_LOBBY).set_x(0);
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

void Drawrer::PrintOnlyCenteredLine(int l, std::string line) const {
  clear();
  PrintCenteredLine(l, line);
  refresh();
}

void Drawrer::PrintCenteredLineColored(int l, std::vector<std::pair<std::string, int>> txt_with_color) {
  // Get total length.
  int total_length = 0;
  for (const auto& it : txt_with_color) 
    total_length += it.first.length();

  // Print parts one by one and update color for each part.
  int position = COLS/2-total_length/2;
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
  PrintCenteredParagraph(paragraph, 0);
}
void Drawrer::PrintCenteredParagraph(texts::paragraph_t paragraph, int additional_height) {
  int counter = 0;
  int start_height = LINES/2 - paragraph.size()/2 - additional_height;
  for (const auto& line : paragraph)
    PrintCenteredLine(start_height+counter++, line);
}

void Drawrer::PrintCenteredParagraphAndMiniFields(texts::paragraph_t paragraph, std::vector<std::string> fields, 
    bool nextmsg) {
  // Add message with availibe commands.
  if (nextmsg) {
    paragraph.push_back("");
    paragraph.push_back("[press 'h' for previous and 'l' for next text. 'q' to quit.]");
  }
  // Print paragraph
  PrintCenteredParagraph(paragraph, 5);

  // Print mini-field(s).
  int start_height = LINES/2 + paragraph.size()/2 + 2;
  for (int r=0; r<10; r++) {
    int start_col = COLS/2-10*fields.size() - 5*(fields.size()/2);
    for (unsigned int field=0; field<fields.size(); field++) {
      const auto& mini_field = mini_fields_.at(fields[field]);
      for (int c=0; c<10; c++) {
        auto sym = mini_field[r][c];
        attron(COLOR_PAIR(sym.color_));
        mvaddstr(start_height+r, start_col++, sym.symbol_.c_str());
        mvaddch(start_height+r, start_col++, ' ');
        attron(COLOR_PAIR(COLOR_DEFAULT));
      }
      start_col += 5;
    }
  }
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
  spdlog::get(LOGGER)->debug("Drawrer::PrintGame");
  std::unique_lock ul(mutex_print_field_);
  if (stop_render_ || !initialized_) 
    return;
  // Print headline
  PrintHeader(update_->audio_played(), update_->PlayersToPrint());
  // Print topline.
  std::vector<bool> topline_colors; 
  if (context == CONTEXT_RESOURCES || context == CONTEXT_TECHNOLOGIES) 
    topline_colors = update_->build_options();
  else if (context == CONTEXT_SYNAPSE) 
    topline_colors = update_->synapse_options();

  if (mode_ != OBSERVER)
    PrintTopline(topline_colors);

  // Print field and/or side-column
  if (!only_side_column)
    PrintField();
  if (!only_field && mode_ != OBSERVER)
    PrintSideColumn();

  // Print footer and message
  if (mode_ != OBSERVER) {
    PrintMessage();
    PrintFooter(cur_selection_.at(cur_view_point_).to_string());
  }
  refresh();
}

void Drawrer::PrintHeader(float audio_played, const t_topline& players) {
  PrintCenteredLineBold(l_headline_, "DISSONANCE");
  ClearLine(l_headline_+1);
  PrintCenteredLineColored(l_headline_+1, players);
  // Print audio
  int length = (c_resources_ - c_field_ - 20) * audio_played;
  for (int i=0; i<length; i++)
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
  position_t sel = field_pos();
  auto range = cur_selection_.at(VP_FIELD).range();

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
          graph_positions_->count(cur) > 0)
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

void Drawrer::PrintSideColumn() {
  attron(WA_BOLD);
  mvaddstr(l_main_, c_resources_, "RESOURCES");
  attroff(WA_BOLD);
  int inc = (static_cast<int>(resources_->size()*2 + technologies_->size()*2+4) > field_height()) ? 1 : 2;
  int counter = 2;
  for (const auto& it : *resources_) {
    // Mark selected resource/ technology
    if (cur_selection_.at(VP_RESOURCE).x() == it.first) {
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
  for (const auto& it : *technologies_) {
    if (cur_selection_.at(VP_TECH).x() == it.first) {
      attron(WA_BOLD);
      attron(COLOR_PAIR(COLOR_AVAILIBLE));
    }
    else if (it.second.first == it.second.second)
      attron(COLOR_PAIR(COLOR_SUCCESS));
    std::string str = units_tech_name_mapping.at(it.first) + " (" + std::to_string(it.second.first) 
      + "/" + std::to_string(it.second.second) + ")";
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
  clear();
  refresh();
  if (show_graph_)
    PrintStatisticsGraph(statistics_[cur_selection_.at(VP_POST_GAME).x()]);
  else
    PrintStatisticsTable(statistics_[cur_selection_.at(VP_POST_GAME).x()]);
}

void Drawrer::PrintStatisticsGraph(std::shared_ptr<Statictics> statistic) const {
  // get most importand infos
  std::vector<StaticticsEntry> graph = statistic->graph();
  
  // Print basic infos
  attron(COLOR_PAIR(statistic->player_color()));
  PrintCenteredLineBold(4, utils::ToUpper("graph " + statistic->player_name()));
  attroff(COLOR_PAIR(statistic->player_color()));
  PrintCenteredLine(5, "('h'/'l' to cycle trough statistics, 'g' for toggeling graph, 'q' to quit.)");

  // Get max value, to calculate compression
  int max = 0;
  for (unsigned int i=0; i<graph.size(); i++) {
    int cur = std::max(std::max(graph[i].oxygen(), graph[i].potassium()), graph[i].glutamate());
    if (cur > max)
      max = cur;
  }
  double fak = (double)(LINES-15)/max; // -10 = 5 spaces top and 5 spaces bottom
  // calculate cols-offset, or reduce graph
  int lines_offset = 7;
  int cols_init_offset = 13;
  int cols_offset = cols_init_offset;
  if ((size_t)(COLS-(cols_offset*2)) > graph.size()) 
    cols_offset += ((COLS-cols_init_offset)-graph.size())/2;
  // Print column discriptions (bottom: time / top: neurons, 
  mvaddstr(LINES-6, cols_offset-cols_init_offset+1, "   (time) ");
  mvaddstr(7, cols_offset-cols_init_offset+1, "(neurons) ");
  // Print legend:
  PrintLegend();

  int max_elems = COLS-(cols_offset*2);
  max_elems -= max_elems*0.07;

  // draw graph-outlines:
  for (int i=0; i<max_elems && i<(int)graph.size(); i++) {
    // y-numbering:
    if (i%10 == 0)
      mvaddstr(LINES-6, i+12+cols_offset, std::to_string(i).c_str());
    // x-numbering:
    if (i%5 == 0 && i<(LINES-16)) {
      int line_val = i/fak;
      mvaddstr(LINES-lines_offset-i, cols_offset, std::to_string(line_val).c_str());
    }
    else if (i<(LINES-16))
      mvaddstr(LINES-lines_offset-i, cols_offset, "|");
    mvaddstr(LINES-lines_offset, i+cols_offset, "-");
  }

  std::vector<double> oxygen;
  std::vector<double> potassium;
  std::vector<double> glutamate;
  std::vector<double> chloride;
  std::vector<double> dopamine;
  std::vector<double> serotonin;
  for (const auto& it : graph) {
    oxygen.push_back(it.oxygen());
    potassium.push_back(it.potassium());
    glutamate.push_back(it.glutamate());
    chloride.push_back(it.chloride());
    dopamine.push_back(it.dopamine());
    serotonin.push_back(it.serotonin());
  }
  auto oxygen_vals = utils::DecimateCurve(oxygen, max_elems);
  auto potassium_vals = utils::DecimateCurve(potassium, max_elems);
  auto glutamate_vals = utils::DecimateCurve(glutamate, max_elems);
  auto chloride_vals = utils::DecimateCurve(chloride, max_elems);
  auto dopamine_vals = utils::DecimateCurve(dopamine, max_elems);
  auto serotonin_vals = utils::DecimateCurve(serotonin, max_elems);

  std::map<int, std::vector<int>> neurons_built;
  for (size_t i=0; i<graph.size(); i++) {
    if (graph[i].neurons_built().size() > 0) 
      neurons_built[i] = graph[i].neurons_built();
  }

  for (int i=0; i<max_elems && i<(int)serotonin_vals.size(); i++) {
    PrintResource(SEROTONIN, (LINES-serotonin_vals[i].second*fak)-lines_offset, i+5+cols_offset);
    if (graph[serotonin_vals[i].first].neurons_built().size()) {
      PrintNeuronsBuilt(neurons_built[serotonin_vals[i].first], i, cols_offset);
      neurons_built.erase(serotonin_vals[i].first);
    }
  }

  for (int i=0; i<max_elems && i<(int)dopamine_vals.size(); i++) {
    PrintResource(DOPAMINE, (LINES-dopamine_vals[i].second*fak)-lines_offset, i+5+cols_offset);
    if (graph[dopamine_vals[i].first].neurons_built().size()) {
      PrintNeuronsBuilt(neurons_built[dopamine_vals[i].first], i, cols_init_offset);
      neurons_built.erase(dopamine_vals[i].first);
    }
  }

  for (int i=0; i<max_elems && i<(int)chloride_vals.size(); i++) {
    PrintResource(CHLORIDE, (LINES-chloride_vals[i].second*fak)-lines_offset, i+5+cols_offset);
    if (graph[chloride_vals[i].first].neurons_built().size()) {
      PrintNeuronsBuilt(neurons_built[chloride_vals[i].first], i, cols_init_offset);
      neurons_built.erase(chloride_vals[i].first);
    }

  }

  for (int i=0; i<max_elems && i<(int)glutamate_vals.size(); i++) {
    PrintResource(GLUTAMATE, (LINES-glutamate_vals[i].second*fak)-lines_offset, i+5+cols_offset);
    if (graph[glutamate_vals[i].first].neurons_built().size()) {
      PrintNeuronsBuilt(neurons_built[glutamate_vals[i].first], i, cols_init_offset);
      neurons_built.erase(glutamate_vals[i].first);
    }

  }

  for (int i=0; i<max_elems && i<(int)potassium_vals.size(); i++) {
    PrintResource(POTASSIUM, (LINES-potassium_vals[i].second*fak)-lines_offset, i+5+cols_offset);
    if (graph[potassium_vals[i].first].neurons_built().size()) {
      PrintNeuronsBuilt(neurons_built[potassium_vals[i].first], i, cols_init_offset);
      neurons_built.erase(potassium_vals[i].first);
    }
  }

  // Also print neurons built with oxygen.
  for (int i=0; i<max_elems && i<(int)oxygen_vals.size(); i++) {
    PrintResource(OXYGEN, (LINES-oxygen_vals[i].second*fak)-lines_offset, i+5+cols_offset);
    if (graph[oxygen_vals[i].first].neurons_built().size()) {
      PrintNeuronsBuilt(neurons_built[oxygen_vals[i].first], i, cols_init_offset);
      neurons_built.erase(serotonin_vals[i].first);
    }
  }
}

void Drawrer::PrintLegend() const {
  for (const auto& it : show_resources_) {
    attron(COLOR_PAIR((it.second) ? it.first : COLOR_DEFAULT));
    std::string msg = std::to_string(it.first) + ": " + resources_name_mapping.at(it.first) 
      + "(" + resource_symbol_mapping.at(it.first) + ")";
    mvaddstr(LINES-3, 10+(it.first-1)*17, msg.c_str());
    attron(COLOR_PAIR(COLOR_DEFAULT));
  }
}

void Drawrer::PrintResource(int resource, int line, int column) const {
  if (show_resources_.at(resource)) {
    attron(COLOR_PAIR(resource));
    mvaddstr(line, column, SYMBOL_FREE);
    attron(COLOR_PAIR(COLOR_DEFAULT));
  }
}

void Drawrer::PrintNeuronsBuilt(std::vector<int> neurons_built, int i, int cols_offset) const {
  size_t size = neurons_built.size();
  for (unsigned int j=0; j<size; j++) {
    size_t neuron = neurons_built[j];
    if (neuron == RESOURCENEURON)
      mvaddstr(7, i+cols_offset-(size-j), "R");
    else 
      mvaddstr(7, i+cols_offset-(size-j), unit_symbol_mapping.at(neuron).c_str());
  }
}

void Drawrer::PrintStatisticsTable(std::shared_ptr<Statictics> statistic) const {
  int start_line = LINES/15;
  // Print player name and "headings" bold. Print only player name in player-color.
  attron(COLOR_PAIR(statistic->player_color()));
  // Player name.
  PrintCenteredLineBold(start_line, utils::ToUpper(statistic->player_name()));
  attroff(COLOR_PAIR(statistic->player_color()));
  PrintCenteredLine(start_line+1, "('h'/'l' to cycle trough statistics, 'g' for graph, 'q' to quit.)");
  int i=3;
  i = PrintStatisticEntry("Neurons Built", start_line, i, statistic->neurons_build());
  i = PrintStatisticEntry("Potentials Built", start_line, i, statistic->potentials_build());
  i = PrintStatisticEntry("Potentials Killed", start_line, i, statistic->potentials_killed());
  i = PrintStatisticEntry("Potentials Lost", start_line, i, statistic->potentials_lost());
  i = PrintStatisticEntry("Enemy Epsps Swallowed By Ipsp", start_line, i, statistic->epsp_swallowed());
  i = PrintStatisticsResources(start_line, i, statistic->stats_resources());
  i = PrintStatisticsTechnology(start_line, i, statistic->stats_technologies());
}

int Drawrer::PrintStatisticEntry(std::string heading, int s, int i, 
    std::map<int, int> infos) const {
  PrintCenteredLineBold(s+(++i), heading);
  for (const auto& it : infos) 
    PrintCenteredLine(s+(++i), units_tech_name_mapping.at(it.first) + ": " + std::to_string(it.second));
  return ++i;
}

int Drawrer::PrintStatisticEntry(std::string heading, int s, int i, int info) const {
  PrintCenteredLineBold(s+(++i), heading);
  PrintCenteredLine(s+(++i), std::to_string(info));
  return ++i;
}

int Drawrer::PrintStatisticsResources(int start_line, int i, 
    std::map<int, std::map<std::string, double>> resources) const {
  PrintCenteredLineBold(start_line+(++i), "Resources");
  for (const auto& resource : resources) {
    PrintCenteredLine(start_line+(++i), resources_name_mapping.at(resource.first));
    std::string info = "";
    for (const auto& jt : resource.second)
      info += jt.first + ": " + utils::Dtos(jt.second) + ", ";
    info.substr(0, info.length()-2);
    PrintCenteredLine(start_line+(++i), info);
  }
  return ++i;
}

int Drawrer::PrintStatisticsTechnology(int start_line, int i, std::map<int, tech_of_t> technologies) const {
  PrintCenteredLineBold(start_line+(++i), "Technologies");
  for (const auto& it : technologies) {
    std::string info = units_tech_name_mapping.at(it.first) + ": " + utils::PositionToString(it.second);
    PrintCenteredLine(start_line+(++i), info);
  }
  return i;
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
  if (lobby_->lobby().size() == 0) {
    PrintCenteredLine(LINES/4+5, "lobby is empty!");
  }
  // Otherwise print lobby
  else {
    int counter = 0;
    for (const auto& it : lobby_->lobby()) {
      std::string lobby_line = it._audio_map_name + ": " + std::to_string(it._cur_players) + "/" 
        + std::to_string(it._max_players);
      if (cur_selection_.at(VP_LOBBY).x() == counter/2)
        attron(COLOR_PAIR(COLOR_AVAILIBLE));
      PrintCenteredLine(LINES/4+5+counter, lobby_line);
      attron(COLOR_PAIR(COLOR_DEFAULT));
      counter += 2;
    }
  }
  refresh();
}

void Drawrer::LoadMiniField(std::string path) {
  nlohmann::json mini_fields_json = utils::LoadJsonFromDisc(path);
  for (const auto& field_json 
      : mini_fields_json.get<std::map<std::string, std::vector<std::vector<nlohmann::json>>>>()) {
    std::vector<std::vector<Data::Symbol>> field;
    for (const auto& it : field_json.second) {
      std::vector<Data::Symbol> field_row;
      for (const auto& jt : it)
        field_row.push_back(Data::Symbol({jt["s"].get<std::string>(), jt["c"].get<int>()}));
      field.push_back(field_row);
    }
    mini_fields_[field_json.first] = field;
  }
}
