#ifndef SRC_PRINT_DRAWER_H_
#define SRC_PRINT_DRAWER_H_

#include <memory>
#include <set>
#include <vector>
#define NCURSES_NOMACROS
#include <cstddef>
#include <curses.h>
#include <shared_mutex>
#include <string>

#include "spdlog/spdlog.h"

#include "client/game/print/view_point.h"
#include "share/shemes/data.h"
#include "share/constants/codes.h"
#include "share/constants/costs.h"
#include "share/constants/texts.h"
#include "share/tools/utils/utils.h"
#include "share/defines.h"

class Drawrer {
  public:
    Drawrer();

    void SetUpBorders(int lines, int cols);

    // getter
    int field_height();
    int field_width();
    position_t field_pos();
    std::string game_id_from_lobby();
    std::vector<bool> synapse_options();

    int GetResource(); 
    int GetTech(); 

    // setter 
    void set_viewpoint(int viewpoint);
    void inc_cur_sidebar_elem(int value); 
    void set_msg(std::string msg);
    void set_transfer(std::shared_ptr<Data> init);
    void set_stop_render(bool stop);
    void set_field_start_pos(position_t pos);
    void set_range(std::pair<position_t, int> range);
    void set_topline(t_topline topline);
    void set_mode(int mode);
    void set_statistics(std::vector<std::shared_ptr<Statictics>> statistics);

    bool InGraph(position_t);
    bool Free(position_t);
    void AddMarker(int type, position_t pos, int color, std::string symbol = "");
    position_t GetMarkerPos(int type, std::string symbol);
    void ClearMarkers(int type = -1);

    /**
     * Adds a new neuron to given position.
     * @param[in] pos
     * @param[in] unit
     */
    void AddNewUnitToPos(position_t pos, short unit, short color);

    void AddTechnology(short technology);

    /**
     * Updates transfer object with new infos.
     * @param[in] transfer_json
     */
    void UpdateTranser(std::shared_ptr<Data> update);

    /**
     * Updates lobby (Locks mutex: unique)
     * param[in] lobby_json
     */
    void UpdateLobby(std::shared_ptr<Data>, bool init=false);

    /**
     * Move viewpoint to next viewpoint
     * @return current context.
     */
    int next_viewpoint();

    /**
     * Clears and refreshes field, locks mutex.
     */
    void ClearField();

    /**
     * Prints a single line centered. (Does not clear screen, does not lock mutex)
     * @param[in] l (what line to print to)
     * @param[in] line (text to print
     */
    void PrintCenteredLine(int l, std::string line) const;
    void PrintCenteredLineBold(int l, std::string line) const;

    /**
     * Clears screen, prints centered line and refreshes screen.
     * @param[in] l (what line to print to)
     * @param[in] line (text to print
     */
    void PrintOnlyCenteredLine(int l, std::string line) const;

    void PrintStatistics() const;

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
     * @param[in] nextmsg adds a message with instructions for prev/next/quit (default=false)
     */
    void PrintCenteredParagraph(texts::paragraph_t paragraph, bool nextmsg=false);
    void PrintCenteredParagraph(texts::paragraph_t paragraph, unsigned int start_height);
    void PrintCenteredParagraphAndMiniFields(texts::paragraph_t paragraph, std::vector<std::string> field, 
        bool nextmsg=false);

    /**
     * Prints mulitple paragraphs to the center of the screen, clearing screen
     * before every new paragraph and locking mutex.
     * @param[in] paragraphs
     */
    void PrintCenteredParagraphs(texts::paragraphs_t paragraph, bool skip_first_wait=false);

    void PrintGame(bool only_field, bool only_side_column, int context);

    /** 
     * Prints lobby (Locks mutex: unique)
     */
    void PrintLobby();

    void CreateMiniFields(int player_color);

  private:
    int lines_;
    int cols_;
    int mode_;

    // transfer data
    bool initialized_;
    std::shared_ptr<Update> update_;
    std::shared_ptr<std::map<int, Data::Resource>> resources_;
    std::shared_ptr<std::map<int, tech_of_t>> technologies_;

    std::vector<std::vector<Data::Symbol>> field_;
    std::shared_ptr<std::set<position_t>> graph_positions_;
    std::shared_ptr<Lobby> lobby_;
    std::vector<std::shared_ptr<Statictics>> statistics_;

    // other data
    std::map<position_t, Data::Symbol> temp_symbols_;
    std::map<int, std::map<position_t, std::pair<std::string, int>>> markers_;
    t_topline topline_;
    std::shared_mutex mutex_print_field_;  ///< mutex locked, when printing field.
    bool stop_render_;
    std::map<std::string, std::vector<std::vector<Data::Symbol>>> mini_fields_;

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
    int extra_height_;  ///< modifies start of field y-coordinate, if size is smaller that screen resolution suggests.
    int extra_width_;  ///< modifies start of field x-coordinate, if size is smaller that screen resolution suggests.

    // start-cols 
    int c_tech_;  ///< start technologies
    int c_field_;  ///< start field
    int c_resources_;  ///< start resources

    // Print methods
    void PrintField();
    void PrintHeader(float audio_played, const t_topline& players);
    void PrintTopline(std::vector<bool> availible_options);
    void PrintTechnologies(const std::string& players);
    void PrintSideColumn();
    void PrintMessage();
    void PrintFooter(std::string str);
    void ClearLine(int line, int start_col=0);

    // helpers
    std::pair<std::string, int> GetReplaceMentFromMarker(position_t pos);
};

#endif
