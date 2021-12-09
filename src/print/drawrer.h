#ifndef SRC_PRINT_DRAWER_H_
#define SRC_PRINT_DRAWER_H_

#include "constants/texts.h"
#include "nlohmann/json.hpp"
#include "objects/transfer.h"
#define NCURSES_NOMACROS

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

    // setter 
    void inc_cur(int value);

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

    void PrintGame(const Transfer& data);

  private:
    int lines_;
    int cols_;

    // Selection
    int cur_;
    int max_resource_;
    int max_tech_;


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
    void PrintFoorer(std::string str);

    std::string ResourceToString(int resource_id, const Transfer::Resource& resource);
};

#endif
