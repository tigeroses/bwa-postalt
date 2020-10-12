
#include <postalt/postalt.h>
#include <postalt/version.h>

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

  postalt::Postalt postalt(alt_filename);
  if (postalt.run())
    std::cout << "Postalt run success!" << std::endl;
  else
    std::cout << "Postalt run fail!" << std::endl;

  return 0;
}
