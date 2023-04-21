project(Munkres)

add_library(Munkres STATIC
  src/Hungarian.cpp
)

target_include_directories(Munkres
  PUBLIC
    src
)
