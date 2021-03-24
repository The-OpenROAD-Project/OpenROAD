project(Munkres)

add_library(Munkres
  src/Hungarian.cpp
)

target_include_directories(Munkres
  PUBLIC
    src
)

