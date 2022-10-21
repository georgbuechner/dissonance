#ifndef SRC_SHARE_TOOLS_AUDIORECEIVER_H_
#define SRC_SHARE_TOOLS_AUDIORECEIVER_H_

#include "share/shemes/data.h"
#include "share/tools/utils/utils.h"
#include "spdlog/spdlog.h"
#include <filesystem>
#include <iostream>
#include <map>
#include <mutex>
#include <shared_mutex>

#define CHUNK_SIZE 4096

class AudioReceiver {
  public: 
    AudioReceiver(std::string user_files_path) : user_files_path_(user_files_path), audio_stored_(false) {}

    // getter 
    std::string audio_file_path() const { return audio_file_path_; }
    std::string audio_file_name() const { return audio_file_name_; }
    const std::string& audio_data() const { return audio_data_; }
    bool audio_stored() const { return audio_stored_; }

    // setter 
    void set_audio_file_name(std::string audio_file_name) { audio_file_name_ = audio_file_name; }
    void set_audio_data(std::string audio_data) { audio_data_ = audio_data; }
    void set_audio_stored(bool audio_stored) { audio_stored_ = audio_stored; }

    /**
     * Adds new audio-data to buffer. 
     * If last part received: a) sets audio-file-name, b) stores data, c)
     * combines sorted data to single buffer. 
     * @param data
     */
    void AddData(std::shared_ptr<Data> data) {
      std::unique_lock ul(mutex_audio_);
      sorted_buffer_.insert(sorted_buffer_.end(), std::pair{data->part(), data->content()});
      // If all data is added to sorted-buffer, store audio and join buffer parts.
      if (sorted_buffer_.size() == (size_t)data->parts()) {
        audio_file_name_ = data->songname();
        // Create directory for this user.
        if (!std::filesystem::exists(user_files_path_ + data->username()))
          std::filesystem::create_directory(user_files_path_ + data->username());
        // Add sorted audio-parts to buffer
        for (const auto& it : sorted_buffer_)
          audio_data_ += it.second;
        audio_file_path_ = user_files_path_ + data->username() + "/" + audio_file_name_;
        utils::StoreMedia(audio_file_path_, audio_data_);
        audio_stored_ = true;
      }
    }

    /**
     * Either takes chunks as received, or generates chunks of CHUNK_SIZE from audio-data.
     * Clears audio_data before returning.
     * @return chunks of CHUNK_SIZE.
     */
    const std::map<int, std::string>& GetCunks() {
      if (audio_stored_ != true || sorted_buffer_.size() == 0) {
        sorted_buffer_.clear(); // make sure buffer is empty.
        while (audio_data_.size() >= CHUNK_SIZE*(sorted_buffer_.size()+1)) {
          sorted_buffer_[sorted_buffer_.size()] = audio_data_.substr(sorted_buffer_.size()*CHUNK_SIZE, CHUNK_SIZE);
        }
        sorted_buffer_[sorted_buffer_.size()] = audio_data_.substr(sorted_buffer_.size()*CHUNK_SIZE, CHUNK_SIZE);
        spdlog::get(LOGGER)->debug("Made {} parts of {} bits data", sorted_buffer_.size(), audio_data_.size());
      }
      audio_data_.clear();
      return sorted_buffer_;
    }

    /**
     * Cleans all audio data. 
     */
    void Clear() { 
      std::unique_lock ul(mutex_audio_);
      spdlog::get(LOGGER)->debug("AudioReceiver::Clear.");
      sorted_buffer_.clear();
      spdlog::get(LOGGER)->debug("AudioReceiver::Clear. cleared buffer");
      audio_data_.clear();
      spdlog::get(LOGGER)->debug("AudioReceiver::Clear. cleared audio-data");
    }

  private: 
    std::shared_mutex mutex_audio_;
    const std::string user_files_path_;
    std::string audio_file_path_;
    std::map<int, std::string> sorted_buffer_;
    std::string audio_file_name_;
    std::string audio_data_;
    bool audio_stored_;
};

#endif
