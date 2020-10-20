cmake_minimum_required(VERSION 3.1)

project(PartClusManager)

set(BIN_NAME "PartClusManager")

option(LOAD_PARTITIONERS "Load MLPart, GPMetis and Chaco" OFF)
if (LOAD_PARTITIONERS)
	add_subdirectory(${PARTCLUSMANAGER_HOME}/Partitioners/Chaco/code)
	add_subdirectory(${PARTCLUSMANAGER_HOME}/Partitioners/Metis)
	add_subdirectory(${PARTCLUSMANAGER_HOME}/Partitioners/MLPart)
endif (LOAD_PARTITIONERS)

set(CMAKE_CXX_STANDARD 11)

#include(${PROJECT_SOURCE_DIR}/cmake/FindTCL.cmake)
find_package(SWIG REQUIRED)

set(PARTCLUSMANAGER_WRAP ${CMAKE_CURRENT_BINARY_DIR}/TclInterfaceWrap.cpp)
add_custom_command(OUTPUT ${PARTCLUSMANAGER_WRAP}
        COMMAND ${SWIG_EXECUTABLE} -namespace -prefix PartClusManager -tcl8 -c++ -o ${PARTCLUSMANAGER_WRAP} ${CMAKE_CURRENT_SOURCE_DIR}/src/partclusmanager.i   
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	DEPENDS ${OPENROAD_HOME}/src/Exception.i

)

set(PARTCLUSMANAGER_TCL_INIT ${CMAKE_CURRENT_BINARY_DIR}/PartClusManagerTclInitVar.cc)
add_custom_command(OUTPUT ${PARTCLUSMANAGER_TCL_INIT}
        COMMAND ${OPENSTA_HOME}/etc/TclEncode.tcl ${PARTCLUSMANAGER_TCL_INIT} partclusmanager_tcl_inits ${PARTCLUSMANAGER_HOME}/src/partclusmanager.tcl
        WORKING_DIRECTORY ${PARTCLUSMANAGER_HOME}/src
        DEPENDS ${PARTCLUSMANAGER_HOME}/src/partclusmanager.tcl ${OPENSTA_HOME}/etc/TclEncode.tcl
)

###########################################################
# Source files
###########################################################

set(SRC_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Hypergraph.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/MakePartclusmanager.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Partitioner.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/HypergraphDecomposition.cpp
   )

###########################################################
# Library
###########################################################

add_library(${BIN_NAME} ${SRC_FILES} ${PARTCLUSMANAGER_WRAP} ${PARTCLUSMANAGER_TCL_INIT})

include_directories(${PARTCLUSMANAGER_HOME}/src
                    PRIVATE
                    ${OPENROAD_HOME}/include
		    ${OPENDB_HOME}/include
                    ${OPENSTA_HOME}/include
                    ${OPENSTA_HOME}/app
		    ${TCL_INCLUDE_PATH}
		    )
if (LOAD_PARTITIONERS)
include_directories(PRIVATE
		    ${PARTCLUSMANAGER_HOME}/Partitioners/Chaco/code
                    ${PARTCLUSMANAGER_HOME}/Partitioners/Metis/include
                    ${PARTCLUSMANAGER_HOME}/Partitioners/MLPart/src
		    )

target_link_libraries(${BIN_NAME} INTERFACE
	Chaco
	metis
	MLPart
	)
endif (LOAD_PARTITIONERS)

target_link_libraries(${BIN_NAME} INTERFACE
			${TCL_LIB}
			)
unset(LOAD_PARTITIONERS)
