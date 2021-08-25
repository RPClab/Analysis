# Channel
root_generate_dictionary(
  G__Channel "${PROJECT_SOURCE_DIR}/include/Channel.hpp" LINKDEF
  "${PROJECT_SOURCE_DIR}/include/ChannelLinkDef.hpp")
add_library(Channel OBJECT "Channel.cpp" G__Channel.cxx)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libChannel_rdict.pcm" TYPE BIN)
set_target_properties(
  Channel PROPERTIES PUBLIC_HEADER "${PROJECT_SOURCE_DIR}/include/Channel.hpp")
target_include_directories(
  Channel
  PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  PUBLIC $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PUBLIC "${ROOT_INCLUDE_DIRS}")
install(TARGETS Channel)

add_library(Channel_static STATIC $<TARGET_OBJECTS:Channel>)
target_include_directories(
  Channel_static
  PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  PUBLIC $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PUBLIC "${ROOT_INCLUDE_DIRS}")
target_link_libraries(Channel_static PUBLIC ${ROOT_LIBRARIES})
install(TARGETS Channel_static)

add_library(Channel_shared SHARED $<TARGET_OBJECTS:Channel>)
target_include_directories(
  Channel_shared
  PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  PUBLIC $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PUBLIC "${ROOT_INCLUDE_DIRS}")
target_link_libraries(Channel_shared PUBLIC ${ROOT_LIBRARIES})
install(TARGETS Channel_shared)

# Event
root_generate_dictionary(
  G__Event "${PROJECT_SOURCE_DIR}/include/Event.hpp" LINKDEF
  "${PROJECT_SOURCE_DIR}/include/EventLinkDef.hpp")
add_library(Event OBJECT "Event.cpp" G__Event.cxx)
target_link_libraries(Event PUBLIC Channel)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libEvent_rdict.pcm" TYPE BIN)
set_target_properties(
  Event PROPERTIES PUBLIC_HEADER "${PROJECT_SOURCE_DIR}/include/Event.hpp")

add_library(Event_static STATIC $<TARGET_OBJECTS:Event>)
target_link_libraries(Event_static PUBLIC Channel_static)
install(TARGETS Event_static)

add_library(Event_shared SHARED $<TARGET_OBJECTS:Event>)
target_link_libraries(Event_shared PUBLIC Channel_shared)
install(TARGETS Event_shared)

add_library(Style STATIC "Style.cpp")
target_link_libraries(Style PUBLIC ${ROOT_LIBRARIES})
target_include_directories(
  Style
  PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  PUBLIC $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PUBLIC "${ROOT_INCLUDE_DIRS}")
install(TARGETS Style)

add_library(Screen STATIC "Screen.cpp")
target_link_libraries(Screen PUBLIC fmt::fmt)
target_include_directories(
  Screen
  PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  PUBLIC $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
install(TARGETS Screen)
