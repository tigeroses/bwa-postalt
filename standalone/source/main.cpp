
#include <postalt/postalt.h>
#include <postalt/version.h>
#include <postalt/workflow.h>

#include <CLI/CLI.hpp>
#include <iostream>
#include <string>
#include <unordered_map>
// #include <spdlog/sinks/basic_file_sink.h>
// #include <spdlog/spdlog.h>

int main(int argc, char** argv) {
  CLI::App app{"Bwa-Postalt: Process sam data."};
  app.footer("Bwa-Postalt version: " + POSTALT_VERSION);
  app.get_formatter()->column_width(40);

  std::string alt_filename;
  app.add_option("alt file", alt_filename, "Position paramter")
      ->check(CLI::ExistingFile)
      ->required();

  CLI11_PARSE(app, argc, argv);

  std::vector<std::string> inputs;
  constexpr int line_size = 1024;
  char buff[line_size];
  while (NULL != fgets(buff, line_size, stdin))
    inputs.push_back(buff);
  std::cerr<<"input size: "<<inputs.size()<<std::endl;
  
  postalt::Postalt postalt(alt_filename);
  std::string outputs;
  postalt.run(inputs, outputs);
  std::cout<<outputs;
  

  // postalt.test_io();
  
  // postalt::Workflow workflow(10, 100*1024*1024);
  // workflow.run();

  return 0;
}
