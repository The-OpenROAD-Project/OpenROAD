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

# Specify the minimum version for CMake
cmake_minimum_required(VERSION 2.8)

# Message
message(STATUS "Global Router CUGR 2.0")
message(STATUS ${CMAKE_CURRENT_SOURCE_DIR})
message(STATUS ${PROJECT_SOURCE_DIR})

# Find includes in the build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Project's name
project(CUGR)

# Set the output folder where your program will be created
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})

set(CMAKE_CXX_STANDARD 14)

set(PATH_RSYN ${CMAKE_CURRENT_SOURCE_DIR}/../rsyn)
set(PATH_FLUTE ${CMAKE_CURRENT_SOURCE_DIR}/../flute)
set(PATH_CUGR ${CMAKE_CURRENT_SOURCE_DIR})

###################
# Warnings/Errors #
###################

set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Werror=return-type")

######################
# Check Dependencies #
######################
set(Boost_USE_STATIC_LIBS ON)
find_package(Boost COMPONENTS system filesystem program_options REQUIRED)
# find_package(Threads)

###############
# Source Code #
###############
file(GLOB_RECURSE SRC_FILES_RSYN
    ${PATH_RSYN}/src/*.cpp
    ${PATH_RSYN}/src/*.cc
    ${PATH_RSYN}/src/*.c)
    
file(GLOB_RECURSE SRC_FILES_FLUTE ${PATH_FLUTE}/*.c)

file(GLOB_RECURSE SRC_FILES_CUGR
    ${PATH_CUGR}/*.cpp
    ${PATH_CUGR}/*.c)

set(SRC_FILES ${SRC_FILES_RSYN} ${SRC_FILES_FLUTE} ${SRC_FILES_CUGR})

#################
# Library Paths #
#################
# Need to come before target is created.

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
link_directories(${PATH_RSYN}/lib/linux)
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
link_directories(${PATH_RSYN}/lib/macosx)
endif()

###########
# Targets #
###########

add_executable(route ${SRC_FILES})

#######################
# Include Directories #
#######################

include_directories(${PATH_RSYN}/src)
include_directories(${PATH_RSYN}/src/rsyn/export)
include_directories(${PATH_RSYN}/include)
include_directories(${PATH_FLUTE})
include_directories(${PATH_CUGR})

#################
# Linker Flags  #
#################

# TODO: set static under release mode only
set_target_properties(route PROPERTIES LINK_FLAGS "-static -Wl,--whole-archive -rdynamic -lpthread -Wl,--no-whole-archive")

# LEF/DEF
target_link_libraries(route lef)
target_link_libraries(route def)

# Boost
target_include_directories(route PUBLIC ${Boost_INCLUDE_DIR})
target_link_libraries(route ${Boost_LIBRARIES})
