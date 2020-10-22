cmake_minimum_required(VERSION 3.1)

project(PartClusManager)

set(BIN_NAME "PartClusManager")

option(LOAD_PARTITIONERS "Load MLPart, GPMetis and Chaco" OFF)

if (LOAD_PARTITIONERS)
	set(PATH_PARTITIONERS ${PARTCLUSMANAGER_HOME} CACHE PATH "Path to Partitioners Folder")
	add_definitions(-DSTUB)
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
		    ${PATH_PARTITIONERS}/Partitioners/Chaco/code
                    ${PATH_PARTITIONERS}/Partitioners/Metis/include
                    ${PATH_PARTITIONERS}/Partitioners/MLPart/src
		    )

find_library(MLPART NAMES MLPart 
	PATHS ${PATH_PARTITIONERS}/Partitioners 
	PATH_SUFFIXES build/MLPart)

if (NOT MLPART)
	add_subdirectory(${PATH_PARTITIONERS}/Partitioners/MLPart MLPart)
	set(MLPART MLPart)
endif()

find_library(CHACO NAMES Chaco 
	PATHS ${PATH_PARTITIONERS}/Partitioners 
	PATH_SUFFIXES build/Chaco/code)

if (NOT CHACO)
	add_subdirectory(${PATH_PARTITIONERS}/Partitioners/Chaco/code Chaco/code)
	set(CHACO Chaco)
endif()

find_library(METIS NAMES metis 
	PATHS ${PATH_PARTITIONERS}/Partitioners 
	PATH_SUFFIXES build/Metis build/Metis/libmetis)

if (NOT METIS)
	add_subdirectory(${PATH_PARTITIONERS}/Partitioners/Metis Metis)
	set(METIS metis)
endif()

target_link_libraries(${BIN_NAME} INTERFACE
		${MLPART}
		${CHACO}
		${METIS}
		)
endif (LOAD_PARTITIONERS)

target_link_libraries(${BIN_NAME} INTERFACE
			${TCL_LIB}
			)
unset(LOAD_PARTITIONERS)
