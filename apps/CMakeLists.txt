include(CLI11)
include(Fmt)

add_executable(Analysis Analysis.cpp)
target_link_libraries(
  Analysis
  PRIVATE Event_static
  PRIVATE Channel_static
  PRIVATE CLI11::CLI11
  PRIVATE fmt::fmt)
install(TARGETS Analysis)
