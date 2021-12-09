#include "print/drawrer.h"
#include "constants/codes.h"
#include "curses.h"
#include "objects/transfer.h"
#include "spdlog/spdlog.h"

#define LOGGER "logger"

#define SIDE_COLUMN_WIDTH 30

Drawrer::Drawrer() { 
  cur_ = 0; 
  max_resource_ = SEROTONIN;
  max_tech_ = NUCLEUS_RANGE-IPSP;
}

int Drawrer::field_height() {
  return l_message_ - l_main_;
}

int Drawrer::field_width() {
  return (c_resources_ - SIDE_COLUMN_WIDTH/2 - 3)/2;
}

void Drawrer::inc_cur(int value) {
  if (value < 0 && cur_ == 1)
    return;
  cur_ = (cur_ + value) % (max_resource_ + max_tech_ + 1);
}

void Drawrer::SetUpBorders(int lines, int cols) {
  // Setup lines 
  int extra_border_lines = 2;
  int lines_field = lines - 8 - 6 - extra_border_lines ; 
  l_headline_ = 1; 
  l_audio_ = 4; 
  l_main_ = 6;
  l_main_ = 8; // implies complete lines for heading = 8
  l_message_ = lines_field + 10;
  l_bottom_ = lines_field + 12;  // height: 4 -> implies complete lines for footer = 6

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

void Drawrer::PrintGame(const Transfer &data) {
  PrintHeader(data.players());
  PrintField(data.field());
  PrintSideColumn(data.resources(), data.technologies());
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
    if (cur_ == it.first) {
      attron(COLOR_PAIR(COLOR_AVAILIBLE));
      PrintFoorer(ResourceToString(it.first, it.second));
    }
    else if (it.second.active_)
      attron(COLOR_PAIR(COLOR_SUCCESS));
    std::string str = codes_name_mapping.at(it.first) + ": " + it.second.value_;
    mvaddstr(l_main_ + counter, c_resources_, str.c_str());
    attron(COLOR_PAIR(COLOR_DEFAULT));
    counter += 2;
  }
  counter += 2;
  mvaddstr(l_main_ + counter, c_resources_, "TECHNOLOGIES");
  counter += 2;
  for (const auto& it : technologies) {
    if (cur_ == it.first-IPSP + max_resource_)
      attron(COLOR_PAIR(COLOR_AVAILIBLE));
    else if (it.second.active_)
      attron(COLOR_PAIR(COLOR_SUCCESS));
    std::string str = codes_name_mapping.at(it.first) + " (" + it.second.cur_ + "/" + it.second.max_ + ")";
    mvaddstr(l_main_ + counter, c_resources_, str.c_str());
    attron(COLOR_PAIR(COLOR_DEFAULT));
    counter += 2;
  }
}

void Drawrer::PrintFoorer(std::string str) {
  PrintCenteredLine(l_bottom_, str);
}

std::string Drawrer::ResourceToString(int resource_id, const Transfer::Resource& resource) {
  return codes_name_mapping.at(resource_id) + ": " + resource.value_ + "+" + resource.bound_ + "/"
    + resource.limit_ + " ++" + resource.iron_; 
}
