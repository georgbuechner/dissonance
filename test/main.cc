#include "spdlog/common.h"
#define CATCH_CONFIG_RUNNER
#include <catch2/catch.hpp>
#include <spdlog/spdlog.h>
#include "spdlog/sinks/basic_file_sink.h"

#define LOGGER "logger"

int main( int argc, char* argv[] ) {
  // global setup...
  auto logger = spdlog::basic_logger_mt("logger", "logs/basic-log.txt");
  spdlog::flush_every(std::chrono::seconds(1));
  spdlog::flush_on(spdlog::level::err);
  spdlog::set_level(spdlog::level::err);

  int result = Catch::Session().run( argc, argv );

  return result;
}


