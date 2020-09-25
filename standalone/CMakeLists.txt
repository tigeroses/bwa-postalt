cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(Bwa-PostaltStandalone LANGUAGES CXX)

# --- Import tools ----

include(../cmake/tools.cmake)

# ---- Dependencies ----

include(../cmake/CPM.cmake)

CPMAddPackage(
  NAME cxxopts
  GITHUB_REPOSITORY jarro2783/cxxopts
  VERSION 2.2.0
  OPTIONS "CXXOPTS_BUILD_EXAMPLES Off" "CXXOPTS_BUILD_TESTS Off"
)

CPMAddPackage(NAME Bwa-Postalt SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/..)

# ---- Create standalone executable ----

file(GLOB sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp)

add_executable(Bwa-PostaltStandalone ${sources})

set_target_properties(Bwa-PostaltStandalone PROPERTIES CXX_STANDARD 17 OUTPUT_NAME "Bwa-Postalt")

target_link_libraries(Bwa-PostaltStandalone Bwa-Postalt cxxopts)
