#include "print/drawrer.h"
#include "constants/codes.h"
#include "curses.h"
#include "objects/transfer.h"
#include "spdlog/spdlog.h"
#include "utils/utils.h"
#include <mutex>

#define LOGGER "logger"

#define SIDE_COLUMN_WIDTH 30
#define BORDER_LINES 2 

#define VP_RESOURCE 1
#define VP_TECH 2

Drawrer::Drawrer() {
  cur_view_point_ = VP_RESOURCE;
  cur_selection_ = {
    {VP_RESOURCE, {IRON, -1, &ViewPoint::inc_resource, &ViewPoint::to_string_resource}},
    {VP_TECH, {WAY, -1, &ViewPoint::inc_tech, &ViewPoint::to_string_tech}}
  };
}

int Drawrer::field_height() {
  return l_message_ - l_main_ - 1;
}

int Drawrer::field_width() {
  return (c_resources_ - SIDE_COLUMN_WIDTH/2 - 3)/2;
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

void Drawrer::inc_cur_sidebar_elem(int value) {
  cur_selection_.at(cur_view_point_).inc(value);
}

int Drawrer::next_viewpoint() {
  cur_view_point_ = (1+cur_view_point_+1)%2 + 1;
  return cur_view_point_;
}

void Drawrer::set_msg(std::string msg) {
  msg_ = msg;
}

void Drawrer::set_transfter(nlohmann::json& data) {
  transfer_ = Transfer(data);
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
  spdlog::get(LOGGER)->flush();
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
  PrintHeader(transfer_.players());
  if (!only_side_column)
    PrintField(transfer_.field());
  if (!only_field)
    PrintSideColumn(transfer_.resources(), transfer_.technologies());
  PrintMessage();
  PrintFooter(cur_selection_.at(cur_view_point_).to_string(transfer_));
  refresh();
}

void Drawrer::PrintHeader(const std::string& players) {
  PrintCenteredLine(l_headline_, "DISSONANCE");
  PrintCenteredLine(l_headline_+1, players);
}

void Drawrer::PrintField(const std::vector<std::vector<Transfer::Symbol>>& field) {
  spdlog::get(LOGGER)->info("Drawrer::PrintField");
  for (unsigned int l=0; l<field.size(); l++) {
    for (unsigned int c=0; c<field[l].size(); c++) {
      attron(COLOR_PAIR(field[l][c].color_));
      mvaddstr(l_main_+l, c_field_ + 2*c, field[l][c].symbol_.c_str());
      mvaddch(l_main_+l, c_field_ + 2*c+1, ' ' );
      attron(COLOR_PAIR(COLOR_DEFAULT));
    }
  }
}

void Drawrer::PrintSideColumn(const std::map<int, Transfer::Resource>& resources,
    const std::map<int, Transfer::Technology>& technologies) {
  mvaddstr(l_main_, c_resources_, "RESOURCES");
  unsigned int counter = 2;
  for (const auto& it : resources) {
    if (cur_selection_.at(VP_RESOURCE).x_ == it.first)
      attron(COLOR_PAIR(COLOR_AVAILIBLE));
    else if (it.second.active_)
      attron(COLOR_PAIR(COLOR_SUCCESS));
    std::string str = resources_name_mapping.at(it.first) + ": " + it.second.value_;
    mvaddstr(l_main_ + counter, c_resources_, str.c_str());
    attron(COLOR_PAIR(COLOR_DEFAULT));
    counter += 2;
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
    counter += 2;
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

void Drawrer::ClearLine(int line) {
  std::string clear_string(COLS, ' ');
  mvaddstr(line, 0, clear_string.c_str());
}
