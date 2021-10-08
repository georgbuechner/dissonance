#include "spdlog/common.h"
#include <filesystem>
#define CATCH_CONFIG_RUNNER
#include <catch2/catch.hpp>
#include <spdlog/spdlog.h>
#include "spdlog/sinks/basic_file_sink.h"
#include "audio/audio.h"

#define LOGGER "logger"

int main( int argc, char* argv[] ) {
  // global setup...
  std::filesystem::remove("test/logs/test-log.txt");

  auto logger = spdlog::basic_logger_mt("logger", "test/logs/test-log.txt");
  spdlog::flush_on(spdlog::level::debug);
  spdlog::set_level(spdlog::level::debug);

  int result = Catch::Session().run( argc, argv );

  return result;
}


