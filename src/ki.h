#ifndef SRC_KI_H_H
#define SRC_KI_H_H

#include <chrono>
#include <mutex>
#include <shared_mutex>

class Ki {
  public:
    Ki( int s_frequenzy = 1250, int t_frequenzy = 10000, int u_frequency = 15000, 
        int s_win = 100, int s_max = 300, int t_win = 100, int t_max = 7500) {
      new_soldier_frequency_ = s_frequenzy;
      new_tower_frequency_ = t_frequenzy;
      update_frequency_ = u_frequency;
      update_soldier_win = s_win;
      soldier_update_max = s_max;
      update_def_win = t_win;
      def_update_max = t_max;

      last_soldier_ = std::chrono::steady_clock::now(); 
      last_def_ = std::chrono::steady_clock::now(); 
      last_update_ = std::chrono::steady_clock::now(); 
    }

    // getter: 
    
    int new_soldier_frequency() const { 
      std::shared_lock sl(mutex_soldier_);
      return new_soldier_frequency_;
    }

    int new_tower_frequency() const {
      std::shared_lock sl(mutex_def_);
      return new_tower_frequency_;
    }

    int update_frequency() const {
      std::shared_lock sl(mutex_update_);
      return update_frequency_;
    }

    std::chrono::time_point<std::chrono::steady_clock> last_soldier() const {
      std::shared_lock sl(mutex_soldier_);
      return last_soldier_;
    }
    std::chrono::time_point<std::chrono::steady_clock> last_def() const {
      std::shared_lock sl(mutex_def_);
      return last_def_;
    }
    std::chrono::time_point<std::chrono::steady_clock> last_update() const {
      std::shared_lock sl(mutex_update_);
      return last_update_;
    }

    // methods
    void reset_last_soldier() { 
      std::unique_lock ul(mutex_soldier_);
      last_soldier_ = std::chrono::steady_clock::now();
    }
    void reset_last_def() { 
      std::unique_lock ul(mutex_def_);
      last_def_ = std::chrono::steady_clock::now();
    }
    void reset_last_update() { 
      std::unique_lock ul(mutex_update_);
      last_update_ = std::chrono::steady_clock::now();
    }

    void update_frequencies() {
      std::unique_lock ul_soldier(mutex_soldier_);
      new_soldier_frequency_ -= update_soldier_win;
      if (new_soldier_frequency_ < soldier_update_max) 
        new_soldier_frequency_ = soldier_update_max;
      ul_soldier.unlock();
      std::unique_lock ul_def(mutex_def_);
      new_tower_frequency_ -= update_def_win;
      if (new_tower_frequency_ < def_update_max)
        new_soldier_frequency_ = def_update_max;
    }

  private:
    int new_soldier_frequency_;
    int new_tower_frequency_;
    int update_frequency_; 
    mutable std::shared_mutex mutex_soldier_;
    mutable std::shared_mutex mutex_def_;
    mutable std::shared_mutex mutex_update_;

    std::chrono::time_point<std::chrono::steady_clock> last_soldier_;
    std::chrono::time_point<std::chrono::steady_clock> last_def_;
    std::chrono::time_point<std::chrono::steady_clock> last_update_;

    int update_soldier_win;
    int soldier_update_max;
    int update_def_win;
    int def_update_max;
};

#endif
