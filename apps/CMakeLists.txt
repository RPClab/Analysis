include(CLI11)
include(Fmt)
include(Rapidcsv)

add_executable(Analysis Analysis.cpp)
target_link_libraries(
  Analysis
  PRIVATE Style
  PRIVATE Event_static
  PRIVATE Channel_static
  PRIVATE CLI11::CLI11
  PRIVATE rapidcsv
  PRIVATE Screen)
install(TARGETS Analysis)

add_executable(Plot Plot.cpp)
target_link_libraries(
  Plot
  PRIVATE Style
  PRIVATE CLI11::CLI11
  PRIVATE ${ROOT_LIBRARIES}
  PRIVATE rapidcsv
  PRIVATE Screen)
target_include_directories(Plot PUBLIC "${ROOT_INCLUDE_DIRS}")
install(TARGETS Plot)
