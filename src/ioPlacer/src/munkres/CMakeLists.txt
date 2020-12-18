project(Munkres)

add_library(Munkres
  STATIC
  src/Hungarian.cpp)

target_include_directories(Munkres
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  )

target_link_libraries(Munkres)
