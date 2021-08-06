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

add_executable(Plot Plot.cpp)
target_link_libraries(
  Plot
  PRIVATE CLI11::CLI11
  PRIVATE ${ROOT_LIBRARIES}
  PRIVATE fmt::fmt)
target_include_directories(Plot PUBLIC "${ROOT_INCLUDE_DIRS}")
install(TARGETS Plot)
