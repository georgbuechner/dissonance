#include "client/client_game.h"
#include "constants/codes.h"
#include "constants/texts.h"
#include "curses.h"
#include "nlohmann/json_fwd.hpp"
#include "objects/transfer.h"
#include "print/drawrer.h"
#include "spdlog/spdlog.h"
#include "utils/utils.h"
#include <vector>

ClientGame::ClientGame(bool relative_size, std::string base_path, std::string username) 
    : username_(username), base_path_(base_path), render_pause_(false), drawrer_() {
  action_ = false;
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
}

nlohmann::json ClientGame::HandleAction(nlohmann::json msg) {
  std::string command = msg["command"];
  nlohmann::json data = msg["data"];

  nlohmann::json response = {{"command", ""}, {"username", username_}, {"data", nlohmann::json()}};

  if (command == "select_mode") {
    response["command"] = "init_game";
    response["data"] = Welcome();
  }
  else if (command == "select_audio") {
    response["command"] = "analyse_audio";
    response["data"]["source_path"] = SelectAudio();
  }
  else if (command == "print_msg") {
    spdlog::get(LOGGER)->info("got command print_msg");
    spdlog::get(LOGGER)->flush();
    drawrer_.ClearField();
    drawrer_.PrintCenteredLine(LINES/2, data["msg"]);
  }
  else if (command == "print_field") {
    drawrer_.PrintGame(Transfer(data));
  }
  else if (command == "distribute_iron") {
    auto resp = DistributeIron(data);
    response["command"] = resp.first;
    response["data"] = resp.second;
    spdlog::get(LOGGER)->debug("Client::HandleAction returning: {}", response.dump());
    spdlog::get(LOGGER)->flush();
  }
  return response;
}


void ClientGame::GetAction() {
  spdlog::get(LOGGER)->info("ClientGame::GetAction.");
  while(true) {
    if (!action_) 
      continue;
    char choice = getch();
    spdlog::get(LOGGER)->info("ClientGame::GetAction action_ {}, in: {}", action_, choice);
    if (!action_) 
      continue;
    if (choice == 'j') {
      drawrer_.inc_cur(1);
    }
    else if (choice == 'k') {
      drawrer_.inc_cur(-1);
    }
  }
}

nlohmann::json ClientGame::Welcome() {
  // Print welcome text.
  drawrer_.PrintCenteredParagraphs(texts::welcome);

  // Select single-player, mulit-player, what ki.
  choice_mapping_t mapping = {{0, {"singe-player", COLOR_AVAILIBLE}}, 
    {1, {"muli-player", COLOR_DEFAULT}}, {2, {"watch ki", COLOR_DEFAULT}}};
    
  auto mode = SelectInteger("Select mode", true, mapping, {mapping.size()+1});

  nlohmann::json data = {{"mode", mode}, {"lines", drawrer_.field_height()}, {"cols", drawrer_.field_width()}, {"base_path", base_path_}};
  return data;
}


// Selection methods

std::pair<std::string, nlohmann::json> ClientGame::DistributeIron(nlohmann::json data) {
  render_pause_ = true;
  action_ = false;
  spdlog::get(LOGGER)->info("ClientGame::DistributeIron.");
  drawrer_.ClearField();
  nlohmann::json resources = data["resources"];
  bool end = false;
  // Get iron and print options.
  drawrer_.PrintCenteredLine(LINES/2-1, "(use 'q' to quit, +/- to add/remove iron and h/l or ←/→ to circle through resources)");
  std::vector<position_t> positions = utils::GetCirclePosition({LINES/2, COLS/2});
  std::string info = "";
  std::string success = "";
  if (data["error"]) 
    attron(COLOR_ERROR);
  else
    attron(COLOR_SUCCESS);
  drawrer_.PrintCenteredLine(LINES/2+2, data["error_msg"]);

  unsigned int current = 0;
  while(!end) {
    // Get current resource and symbol.
    std::string current_symbol = resource_symbols[current];
    spdlog::get(LOGGER)->info("ClientGame::DistributeIron current symbol: {}", current_symbol);
    int resource = resources_symbol_mapping.at(current_symbol);

    // Print texts (help, current resource info)
    drawrer_.PrintCenteredLine(LINES/2-2, data["help"]);
    info = resources_name_mapping.at(resource) + ": FE" 
      + std::to_string(resources[resource]["iron"].get<int>())
      + ((resources[resource]["active"]) ? " (active)" : " (inactive)");
    drawrer_.PrintCenteredLine(LINES/2+1, info);

    // Print resource circle.
    for (unsigned int i=0; i<resource_symbols.size(); i++) {
      if (resources[resources_symbol_mapping.at(resource_symbols[i])]["active"])
        attron(COLOR_PAIR(COLOR_SUCCESS));
      if (i == current)
        attron(COLOR_PAIR(COLOR_MARKED));
      mvaddstr(positions[i].first, positions[i].second, resource_symbols[i].c_str());
      attron(COLOR_PAIR(COLOR_DEFAULT));
      refresh();
    }

    // Get players choice.
    char choice = getch();

    // Move clockwise 
    if (utils::IsDown(choice))
      current = utils::Mod(current+1, resource_symbols.size());
    // Move counter-clockwise 
    else if (utils::IsUp(choice))
      current = utils::Mod(current-1, resource_symbols.size());
    // Add iron 
    else if (choice == '+') 
      return {"add_iron", {{"resource", (resources_symbol_mapping.count(current_symbol) > 0) 
        ? resources_symbol_mapping.at(current_symbol) : Resources::OXYGEN}}};
    // Remove iron 
    else if (choice == '-') {
      return {"remove_iron", {{"resource", (resources_symbol_mapping.count(current_symbol) > 0) 
        ? resources_symbol_mapping.at(current_symbol) : Resources::OXYGEN}}};
    }
    // Exit
    else if (choice == 'q') {
      end = true;
    }
    spdlog::get(LOGGER)->info("ClientGame::DistributeIron choice: {}", choice);
  }
  drawrer_.ClearField();
  render_pause_ = false;
  action_ = true;
  return {"", {}};
}

int ClientGame::SelectInteger(std::string msg, bool omit, choice_mapping_t& mapping, std::vector<size_t> splits) {
  // TODO(fux): check whether to pause game or not `pause_ = true;`
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
      drawrer_.PrintCenteredLine(LINES/2+counter+5, "Selection not available (not enough resources?): " 
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
