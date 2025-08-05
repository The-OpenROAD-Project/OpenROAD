#########
# Setup #
#########

add_library(CUGR
  gr/GlobalRouter.cpp
  gr/GridGraph.cpp
  gr/GRNet.cpp
  gr/GRTree.cpp
  gr/MazeRoute.cpp
  gr/PatternRoute.cpp
  obj/Design.cpp
  obj/GeoTypes.cpp
  obj/Instance.cpp
  obj/Layers.cpp
  obj/Net.cpp
  utils/log.cpp
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
    utl_lib
    stt_lib
    odb
    Boost::boost)
