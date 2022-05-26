#ifndef SRC_CLIENT_CONTEXT_H_
#define SRC_CLIENT_CONTEXT_H_

#include "share/defines.h"
#include "share/shemes/data.h"
#include "share/tools/eventmanager.h"
#include "share/constants/texts.h"
#include <memory>
#include <vector>

class ClientGame;

class Context {
  public: 
    /**
     * Empty constructor for use in f.e. maps.
     */
    Context() : msg_("") {}

    /**
     * Default constructor with one set of initial handlers.
     * @param[in] handlers to be added.
     */
    Context(std::string msg, std::map<char, void(ClientGame::*)(std::shared_ptr<Data> data)>& handlers,
        std::vector<std::pair<std::string, int>> topline) : msg_(msg) {
      for (const auto& it : handlers)
        eventmanager_.AddHandler(it.first, it.second);
      topline_ = topline;
      data_ = std::make_shared<Data>();
    } 

    /**
     * Constructor with two sets of initial handlers. (Mainly to make
     * initializing easier, if multiple contexts use a set of identical
     * handlers.
     * @param[in] std_handlers to be added (standard handlers)
     * @param[in] handlers to be added (special handlers).
     */
    Context(std::string msg, std::map<char, void(ClientGame::*)(std::shared_ptr<Data> data)> std_handlers, 
        std::map<char, void(ClientGame::*)(std::shared_ptr<Data> data)> handlers,
        std::vector<std::pair<std::string, int>> topline) : msg_(msg) {
      for (const auto& it : std_handlers)
        eventmanager_.AddHandler(it.first, it.second);
      for (const auto& it : handlers)
        eventmanager_.AddHandler(it.first, it.second);
      topline_ = topline;
      data_ = std::make_shared<Data>();
    } 

    // getter
    std::string msg() const { return msg_; }
    int current_unit() const { return current_unit_; }
    int current_range() const { return current_range_; }
    position_t current_pos() const { return current_pos_; }
    char cmd() const { return cmd_; }
    std::shared_ptr<Data> data() const { return data_; }
    std::string action() const { return action_; }
    texts::paragraphs_field_t text() const { return text_; }
    unsigned int num() const { return num_; }
    int last_context() const { return last_context_; }

    std::vector<std::pair<std::string, int>> topline() const { return topline_; }
    EventManager<char, ClientGame, std::shared_ptr<Data>>& eventmanager() { return eventmanager_; }

    // setter 
    void set_data(std::shared_ptr<Data> data) { data_ = data; }
    void set_cmd(char cmd) { cmd_ = cmd; }
    void set_action(std::string action) { action_ = action; }
    void set_text(texts::paragraphs_field_t text) { text_ = text; }
    void set_num(unsigned int num) { num_ = num; }
    void set_last_context(int context) { last_context_ = context; }

    // methods 
    void init_text(texts::paragraphs_field_t text, int context) {
      text_ = text;
      last_context_ = context;
      num_ = 0;
    }
    texts::paragraph_field_t get_paragraph() { return text_[num_]; }
    bool last_text() { return text_.size()-1 == num_; }
    
  private:
    EventManager<char, ClientGame, std::shared_ptr<Data>> eventmanager_;
    std::string action_;
    std::string msg_;
    int current_unit_;
    int current_range_;
    position_t current_pos_;

    char cmd_;
    std::shared_ptr<Data> data_;

    texts::paragraphs_field_t text_;
    unsigned int num_;
    int last_context_;

    t_topline topline_;
};

#endif
