#ifndef SRC_PRINT_DRAWER_H_
#define SRC_PRINT_DRAWER_H_

#include "nlohmann/json_fwd.hpp"
#define NCURSES_NOMACROS
#include "constants/texts.h"
#include "nlohmann/json.hpp"
#include "objects/transfer.h"
#include "constants/codes.h"
#include "utils/utils.h"
#include <string>

#include <cstddef>
#include <curses.h>

#define COLOR_AVAILIBLE 1
#define COLOR_ERROR 2 
#define COLOR_DEFAULT 3
#define COLOR_MSG 4
#define COLOR_SUCCESS 5
#define COLOR_MARKED 6
#define COLOR_PROGRESS 7

class Drawrer {
  public:
    Drawrer();

    void SetUpBorders(int lines, int cols);

    // getter
    int field_height();
    int field_width();

    int GetResource(); 
    int GetTech(); 

    // setter 
    void inc_cur_sidebar_elem(int value); void set_msg(std::string msg);
    void next_viewpoint();
    void set_transfter(nlohmann::json& data);

    /**
     * Clears and refreshes field, locks mutex.
     */
    void ClearField();

    /**
     * Prints a single line centered. (Does not clear screen, does not lock mutex)
     * @param[in] l (what line to print to)
     * @param[in] line (text to print
     */
    void PrintCenteredLine(int l, std::string line);

    /**
     * Prints a single line centered, but here the line is splitted into parts,
     * with each part assined it's own color. (Does not clear screen, does not lock mutex)
     * @param[in] l line to print to
     * @param[in] txt_with_color array of text parts, each with a color.
     */
    void PrintCenteredLineColored(int line, std::vector<std::pair<std::string, int>> txt_with_color);

    /**
     * Prints a paragraph (mulitple lines of text) to the center of the screen.
     * (Does not clear screen, does not lock mutex)
     * @param[in] paragraph
     */
    void PrintCenteredParagraph(texts::paragraph_t paragraph);

    /**
     * Prints mulitple paragraphs to the center of the screen, clearing screen
     * before every new paragraph and locking mutex.
     * @param[in] paragraphs
     */
    void PrintCenteredParagraphs(texts::paragraphs_t paragraph);

    void PrintGame(bool only_field, bool only_side_column);

  private:
    int lines_;
    int cols_;
    Transfer transfer_;

    // Selection
    struct ViewPoint {
      public: 
        ViewPoint(int x, int y, void(ViewPoint::*f_inc)(int val), 
            std::string(ViewPoint::*f_ts)(const Transfer&)) : x_(x), y_(y), inc_(f_inc), to_string_(f_ts) {}

        int x_;
        int y_;
        void inc(int val) { (this->*inc_)(val); }
        std::string to_string(const Transfer& transfer) { return(this->*to_string_)(transfer); }

        void inc_resource(int val) { x_= utils::Mod(x_+val, SEROTONIN+1); }
        void inc_tech(int val) { x_= utils::Mod(x_+val, NUCLEUS_RANGE+1, WAY); }

        std::string to_string_resource(const Transfer& transfer) {   
          Transfer::Resource resource = transfer.resources().at(x_);
          return resources_name_mapping.at(x_) + ": " + resource.value_ + "+" 
            + resource.bound_ + "/" + resource.limit_ + " ++" + resource.iron_
            + "$" + resource_description_mapping.at(x_);
        }

        std::string to_string_tech(const Transfer& transfer) {
          Transfer::Technology tech = transfer.technologies().at(x_);
          return units_tech_name_mapping.at(x_) + ": " + tech.cur_ + "/" + tech.max_ 
            + "$" + units_tech_description_mapping.at(x_);
        }

      private:
        void(ViewPoint::*inc_)(int val);
        std::string(ViewPoint::*to_string_)(const Transfer&);
    };
    std::map<int, ViewPoint> cur_selection_; // 0: map, 1: resource, 2: tech
    int cur_view_point_;

    // Boxes
    std::string msg_;


    // start-lines header
    int l_headline_;  ///< start of headline showing most essential game infos
    int l_audio_;  ///< start of audio information
    int l_bar_;  ///< start of help-bar
    int l_main_;  ///< start of main-content
    int l_message_;  ///< start of quick-message
    int l_bottom_;  ///< start of footer

    // start-cols 
    int c_tech_;  ///< start technologies
    int c_field_;  ///< start field
    int c_resources_;  ///< start resources

    // Print methods
    void PrintField(const std::vector<std::vector<Transfer::Symbol>>& field);
    void PrintHeader(const std::string& players);
    void PrintTechnologies(const std::string& players);
    void PrintSideColumn(const std::map<int, Transfer::Resource>& resources, 
        const std::map<int, Transfer::Technology>& technologies);
    void PrintMessage();
    void PrintFooter(std::string str);
};

#endif
