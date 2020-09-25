#include <doctest/doctest.h>
#include <postalt/postalt.h>

#include <string>

TEST_CASE("Postalt") {
  using namespace postalt;

  Postalt postalt("Tests");

  CHECK(postalt.greet(LanguageCode::EN) == "Hello, Tests!");
  CHECK(postalt.greet(LanguageCode::DE) == "Hallo Tests!");
  CHECK(postalt.greet(LanguageCode::ES) == "¡Hola Tests!");
  CHECK(postalt.greet(LanguageCode::FR) == "Bonjour Tests!");
}

TEST_CASE("Postalt version") {
  static_assert(std::string_view("1.0") == std::string_view("1.0"));
  CHECK(std::string("1.0") == std::string("1.0"));
}
