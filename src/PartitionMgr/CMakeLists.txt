cmake_minimum_required(VERSION 3.1)

project(PartitionMgr)

set(BIN_NAME "PartitionMgr")

option(LOAD_PARTITIONERS "Load MLPart, GPMetis and Chaco" OFF)

if (LOAD_PARTITIONERS)
        set(PATH_PARTITIONERS ${PARTITIONMGR_HOME} CACHE PATH "Path to Partitioners Folder")
        add_definitions(-DPARTITIONERS)
endif (LOAD_PARTITIONERS)

#include(${PROJECT_SOURCE_DIR}/cmake/FindTCL.cmake)
find_package(SWIG REQUIRED)

set(PARTITIONMGR_WRAP ${CMAKE_CURRENT_BINARY_DIR}/PartitionMgrWrap.cpp)
add_custom_command(OUTPUT ${PARTITIONMGR_WRAP}
        COMMAND ${SWIG_EXECUTABLE} -namespace -prefix par -tcl8 -c++ -o ${PARTITIONMGR_WRAP} ${CMAKE_CURRENT_SOURCE_DIR}/src/partitionmgr.i   
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        DEPENDS ${OPENROAD_HOME}/src/Exception.i

)

set(PARTITIONMGR_TCL_INIT ${CMAKE_CURRENT_BINARY_DIR}/PartitionMgrTclInitVar.cc)
add_custom_command(OUTPUT ${PARTITIONMGR_TCL_INIT}
        COMMAND ${OPENSTA_HOME}/etc/TclEncode.tcl ${PARTITIONMGR_TCL_INIT} partitionmgr_tcl_inits ${PARTITIONMGR_HOME}/src/partitionmgr.tcl
        WORKING_DIRECTORY ${PARTITIONMGR_HOME}/src
        DEPENDS ${PARTITIONMGR_HOME}/src/partitionmgr.tcl ${OPENSTA_HOME}/etc/TclEncode.tcl
)

###########################################################
# Source files
###########################################################

set(SRC_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Hypergraph.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/MakePartitionMgr.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/PartitionMgr.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/HypergraphDecomposition.cpp
   )

###########################################################
# Library
###########################################################

add_library(${BIN_NAME} ${SRC_FILES} ${PARTITIONMGR_WRAP} ${PARTITIONMGR_TCL_INIT})

include_directories(${PARTITIONMGR_HOME}/src
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

target_link_libraries(${BIN_NAME}
			PUBLIC
			utility
                        )
unset(LOAD_PARTITIONERS)
