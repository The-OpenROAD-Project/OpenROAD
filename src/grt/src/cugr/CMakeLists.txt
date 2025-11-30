#########
# Setup #
#########

add_library(CUGR
  src/CUGR.cpp
  src/Design.cpp
  src/GeoTypes.cpp
  src/GRNet.cpp
  src/GRTree.cpp
  src/GridGraph.cpp
  src/Layers.cpp
  src/Netlist.cpp
  src/MazeRoute.cpp
  src/PatternRoute.cpp
)

target_compile_options(CUGR
  PRIVATE
    -Wno-array-bounds
)

target_include_directories(CUGR
  PUBLIC
    include
  PRIVATE
    # Ugly but necessary to get GRoute.h and avoid a circular dependency
    # with FastRoute.  Remove once this data moves to OpenDB
    ../../include
)

target_link_libraries(CUGR
  PRIVATE
    dbSta_lib
    utl_lib
    stt_lib
    odb
    Boost::boost)
