
#include <doctest/doctest.h>
#include <postalt/altfile.h>

using namespace postalt;

TEST_CASE("Altfile")
{
    Altfile altfile("test.alt");
    altfile.parse();
}