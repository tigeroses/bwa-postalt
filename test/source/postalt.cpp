
#include <doctest/doctest.h>
#include <postalt/postalt.h>
#include <postalt/version.h>

#include <string>

TEST_CASE("Postalt") {
  using namespace postalt;

  Postalt postalt("Tests");

  CHECK(postalt.run());
}

TEST_CASE("Postalt version") { CHECK(std::string(POSTALT_VERSION) == std::string("1.0.0")); }
