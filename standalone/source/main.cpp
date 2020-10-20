
#include <postalt/postalt.h>
#include <postalt/version.h>
#include <postalt/workflow.h>
#include <postalt/timer.h>

#include <iostream>
#include <string>
#include <unordered_map>
#include <thread>
#include <sstream>
#include <filesystem>

#include <CLI/CLI.hpp>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

int main(int argc, char** argv) {
  CLI::App app{"Bwa-Postalt: Process sam data."};
  app.footer("Bwa-Postalt version: " + POSTALT_VERSION);
  app.get_formatter()->column_width(40);

  std::string alt_filename;
  app.add_option("alt file", alt_filename, "Position paramter")
      ->check(CLI::ExistingFile)
      ->required();

  int core_num = std::thread::hardware_concurrency();
  app.add_option("-c", core_num, "Core number, default detect")
      ->check(CLI::PositiveNumber);

  int buffer_size = 1000 * 1000;
  app.add_option("-b", buffer_size, "Line number of single buffer, default 1M")
    ->check(CLI::PositiveNumber);

  std::string log_path = "logs";
  app.add_option("--log", log_path, "Set logging path, default './logs'");

  CLI11_PARSE(app, argc, argv);

  // Set the default logger to file logger.
  std::time_t        t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  std::ostringstream ostr;
  ostr << "Bwa-Postalt_" << std::put_time(std::localtime(&t), "%Y%m%d_%H%M%S") << ".log";
  try
  {
      // auto file_logger = spdlog::basic_logger_mt("main", "logs/" + ostr.str());
      std::filesystem::path fs_log_path(log_path);
      auto     file_sink      = std::make_shared< spdlog::sinks::basic_file_sink_mt >(fs_log_path / ostr.str());
      auto     main_logger    = std::make_shared< spdlog::logger >("main", file_sink);
      auto     process_logger = std::make_shared< spdlog::logger >("process", file_sink);
      spdlog::register_logger(main_logger);
      spdlog::register_logger(process_logger);
      spdlog::set_default_logger(process_logger);
  }
  catch (const spdlog::spdlog_ex& ex)
  {
      std::cerr << "Log init failed: " << ex.what() << std::endl;
  }
  spdlog::set_level(spdlog::level::info);  // Set global log level.
  spdlog::flush_on(spdlog::level::info);
  spdlog::set_pattern("%Y-%m-%d %H:%M:%S.%e %L %n: %v");

  // Test single thread
  // std::vector<std::string> inputs;
  // constexpr int line_size = 1024;
  // char buff[line_size];
  // while (NULL != fgets(buff, line_size, stdin))
  //   inputs.push_back(buff);
  // std::cerr<<"input size: "<<inputs.size()<<std::endl;
  
  // postalt::Postalt postalt(alt_filename);
  // std::string outputs;
  // postalt.run(inputs, outputs);
  // std::cout<<outputs;
  
  // Test IO
  // postalt.test_io();
  
  spdlog::get("main")->info("Start processing...");

  Timer timer;
  postalt::Workflow workflow(core_num, buffer_size);
  workflow.run(alt_filename);

  spdlog::get("main")->info("End processing, elapsed time(s): {:.2f}", timer.toc(1000));

  return 0;
}
