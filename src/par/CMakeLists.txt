# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021-2025, The OpenROAD Authors

include("openroad")

option(LOAD_CPLEX "Load CPLEX" OFF)
if (LOAD_CPLEX)
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_MODULE_PATH               "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
  set(CMAKE_CXX_FLAGS                 "${CMAKE_CXX_FLAGS} -DIL_STD -m64 -Wall -fPIC")
  set(CMAKE_INCLUDE_SYSTEM_FLAG_CXX   "-isystem ")
  set(LINKER_OPTIONS                  "-Wl,--export-dynamic")
  find_package(Cplex)
  include_directories(SYSTEM ${CPLEX_INCLUDE_DIRS})
endif (LOAD_CPLEX)

find_package(Threads REQUIRED)
find_package(ortools REQUIRED)

add_library(par_lib
  src/PartitionMgr.cpp
  src/TritonPart.cpp
  src/Hypergraph.cpp
  src/Utilities.cpp
  src/Coarsener.cpp
  src/Multilevel.cpp
  src/Refiner.cpp
  src/Partitioner.cpp
  src/Evaluator.cpp
  src/GreedyRefine.cpp
  src/ILPRefine.cpp
  src/KWayFMRefine.cpp
  src/KWayPMRefine.cpp
  src/PriorityQueue.cpp
)

target_include_directories(par_lib
  PUBLIC
    include
)

target_link_libraries(par_lib
  PRIVATE
    odb
    OpenSTA
    utl_lib
    dbSta_lib
    ortools::ortools
)

if (LOAD_CPLEX)
  target_compile_definitions(par_lib
    PRIVATE
      LOAD_CPLEX
  )

  target_link_libraries(par_lib
    PRIVATE
      ${CPLEX_LIBRARIES}
      ${CMAKE_THREAD_LIBS_INIT}
      dl
  )
endif (LOAD_CPLEX)


swig_lib(NAME      par
         NAMESPACE par
         I_FILE    src/partitionmgr.i
         SCRIPTS   src/partitionmgr.tcl
)

target_sources(par
  PRIVATE
    src/MakePartitionMgr.cpp
)

messages(
  TARGET par_lib
)

target_include_directories(par
  PUBLIC
    include
)

target_link_libraries(par
  PRIVATE
    odb
    OpenSTA
    par_lib
)

if (LOAD_CPLEX)
  target_compile_definitions(par
    PRIVATE
      LOAD_CPLEX
  )

  target_link_libraries(par
    PRIVATE
      ${CPLEX_LIBRARIES}
      ${CMAKE_THREAD_LIBS_INIT}
      dl
  )
endif (LOAD_CPLEX)


if (Python3_FOUND AND BUILD_PYTHON)
  swig_lib(NAME          par_py
           NAMESPACE     par
           LANGUAGE      python
           I_FILE        src/partitionmgr-py.i
           SWIG_INCLUDES ${PROJECT_SOURCE_DIR}/include/par
           SCRIPTS       ${CMAKE_CURRENT_BINARY_DIR}/par_py.py
  )

  target_include_directories(par_py
    PUBLIC
      include
  )

  target_link_libraries(par_py
    PUBLIC
      par_lib
      utl_lib
  )

endif()

add_subdirectory(test)
