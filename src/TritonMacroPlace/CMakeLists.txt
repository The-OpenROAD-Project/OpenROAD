set(THREADS_PREFER_PTHREAD_FLAG ON)

set(ABKCOMMON_HOME ${MPLACE_HOME}/module/ABKCommon )
set(PARQUET_HOME ${MPLACE_HOME}/module/ParquetFP )

add_subdirectory(${ABKCOMMON_HOME})
add_subdirectory(${PARQUET_HOME})

find_package(Eigen3 REQUIRED)

# for cimg
find_package(Threads REQUIRED)


############################################################
# Create a library 
############################################################

set(MPLACE_WRAP ${CMAKE_CURRENT_BINARY_DIR}/MacroPlace_wrap.cc)
set(MPLACE_TCL_INIT ${CMAKE_CURRENT_BINARY_DIR}/MacroPlaceTclInitVar.cc)


add_custom_command(OUTPUT ${MPLACE_WRAP}
  COMMAND ${SWIG_EXECUTABLE} -tcl8 -c++ -namespace -prefix mpl -o ${MPLACE_WRAP} -I${MPLACE_HOME}/include ${MPLACE_HOME}/src/MacroPlace.i   
  WORKING_DIRECTORY ${MPLACE_HOME}
  DEPENDS ${MPLACE_HOME}/src/MacroPlace.i "src/TritonMacroPlace.cpp" "include/mpl/TritonMacroPlace.h"
)

add_custom_command(OUTPUT ${MPLACE_TCL_INIT}
  COMMAND ${OPENSTA_HOME}/etc/TclEncode.tcl ${MPLACE_TCL_INIT} macro_placer_tcl_inits ${MPLACE_HOME}/src/MacroPlace.tcl
  WORKING_DIRECTORY ${MPLACE_HOME}/src
  DEPENDS ${MPLACE_HOME}/src/MacroPlace.tcl ${OPENSTA_HOME}/etc/TclEncode.tcl
)

############################################################
# Create an library 
############################################################
add_library( TritonMacroPlace 
  src/circuit.cpp
  src/Partition.cpp
  src/macro.cpp
  src/TritonMacroPlace.cpp
  src/MacroPlace.cpp
  src/MakeMacroPlace.cpp
  ${MPLACE_WRAP}
  ${MPLACE_TCL_INIT}
)

############################################################
# include directories 
############################################################

target_include_directories( TritonMacroPlace 
PUBLIC
  ${MPLACE_HOME}/src
  ${MPLACE_HOME}/include
  utility 

PRIVATE
  ${ABKCOMMON_HOME}
  ${PARQUET_HOME}/src

  ${OPENSTA_HOME}/include

  ${OPENDB_HOME}/include
  ${OPENROAD_HOME}/include
  ${DBSTA_HOME}/include

  ${TCL_INCLUDE_PATH}
  )

target_link_libraries( TritonMacroPlace
  Eigen3::Eigen
  utility
  )
