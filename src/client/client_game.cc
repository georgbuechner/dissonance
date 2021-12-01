#include "client/client_game.h"
#include "constants/texts.h"

ClientGame::ClientGame(bool relative_size, std::string base_path) : base_path_(base_path) {
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
  lines_ = 40;
  cols_  = 74;
  left_border_ = (COLS - cols_) /2 - 40;
  if (relative_size) {
    lines_ = LINES-20;
    cols_ = (COLS-40)/2;
    left_border_ = 10;
  }

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

}

void ClientGame::Welcome() {
  std::cout << "WELCOME!!" << std::endl;

  // Check termin size and print warning in case size is insufficient for standard size.
  if (LINES < lines_+20 || COLS < (cols_*2)+40) { 
    texts::paragraphs_t paragraphs = {{
        {"Your terminal size is to small to play in standard size."},
        {"Expected width: " + std::to_string(cols_*2+50) + ", actual: " + std::to_string(COLS)},
        {"Expected hight: " + std::to_string(lines_+20) + ", actual: " + std::to_string(LINES)},
        {},
        {"Either increase terminal size, or run `dissonance -r`"},
        {"(Enter play anyway, q to quit)"}
    }};
    PrintCenteredParagraphs(paragraphs);
    char c = getch();
    if (c == 'q')
      return;
  }
  
  // Print welcome text.
  PrintCenteredParagraphs(texts::welcome);
}

// print methods

void ClientGame::ClearField() {
  std::unique_lock ul(mutex_print_);
  clear();
  refresh();
}

void ClientGame::PrintCenteredLine(int line, std::string txt) {
  std::string clear_string(COLS, ' ');
  mvaddstr(line, 0, clear_string.c_str());
  mvaddstr(line, COLS/2-txt.length()/2, txt.c_str());
}

void ClientGame::PrintCenteredParagraph(texts::paragraph_t paragraph) {
    int size = paragraph.size()/2;
    int counter = 0;
    for (const auto& line : paragraph)
      PrintCenteredLine(LINES/2-size+(counter++), line);
}

void ClientGame::PrintCenteredParagraphs(texts::paragraphs_t paragraphs) {
  for (auto& paragraph : paragraphs) {
    ClearField();
    paragraph.push_back({});
    paragraph.push_back("[Press any key to continue]");
    PrintCenteredParagraph(paragraph);
    getch();
  }
}
