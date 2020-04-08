project(OpenPhySyn VERSION 1.2.4 LANGUAGES CXX)


option(OPENPHYSYN_WARNINGS_SETTINGS_ENABLED "Allow target_set_warnings to add flags and defines.
                                 Set this to OFF if you want to provide your own warning parameters." ON)
option(OPENPHYSYN_LTO_ENABLED "Enable link time optimization" ON)
option(OPENPHYSYN_UNIT_TESTS_ENABLED "Include tests in the library. Setting this to OFF will remove all doctest related code.
                        Tests in tests/*.cpp will still be enabled." OFF)
option(OPENPHYSYN_STANDARD_TRANSFORMS_ENABLED "Build OpenPhySyn Standard Transforms" ON)
option(OPENPHYSYN_TRANSFORM_HELLO_TRANSFORM_ENABLED "Build demo hello transform" OFF)
option(OPENPHYSYN_TRANSFORM_BUFFER_FANOUT_ENABLED "Build Buffer Fan-Out transform" ON)
option(OPENPHYSYN_TRANSFORM_GATE_CLONE_ENABLED "Build Gate Clone transform" ON)
option(OPENPHYSYN_TRANSFORM_PIN_SWAP_ENABLED "Build Pin Swap transform" ON)
option(OPENPHYSYN_TRANSFORM_CONSTANT_PROPAGATION_ENABLED "Build Constant Propagation transform" ON)

set(LIBRARY_NAME ${PROJECT_NAME})  # Default name for the library built from src/*.cpp (change if you wish)
# string(TOLOWER ${PROJECT_NAME} LIBRARY_NAME)


set(OPENPHYSYN_LINKED_TRANSFORMS "")
set(OPENPHYSYN_TRANSFORMS_LINKING_LIBRARY ${LIBRARY_NAME})


set(OPENPHYSYN_CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OPENPHYSYN_CMAKE_CXX_FLAGS} -pthread -Wno-error -Wno-deprecated")
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  set(OPENPHYSYN_CMAKE_CXX_FLAGS "${OPENPHYSYN_CMAKE_CXX_FLAGS} -rdynamic -Wl,-export_dynamic -Wno-unused-command-line-argument")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(OPENPHYSYN_CMAKE_CXX_FLAGS "${OPENPHYSYN_CMAKE_CXX_FLAGS} -rdynamic")
endif()


# Tool build configuration
set(OPENPHYSYN_DATABASE_HANDLER "OpenSTA" CACHE STRING "Specify the database handler type, can be OpenDB or OpenSTA")


add_definitions(-DOPENPHYSYN_AUTO_LINK)
set(OPENPHYSYN_TRANSFORMS_LINKING_LIBRARY "")

set(SWIG_FLAGS "-w467,72,503,509")
set(SWIG_PSN_LIB_FLAGS "")
set(SWIG_FLAGS "-DSWIG_DIRECTOR_STATIC ${SWIG_FLAGS}")
set(SWIG_PSN_LIB_FLAGS "-DOPENROAD_OPENPHYSYN_LIBRARY_BUILD")


set(PSN_HOME
  ${PROJECT_SOURCE_DIR}
)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PSN_HOME}/cmake")

# Default to bulding optimnized/release executable.
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RELEASE)
endif()

if(${OPENPHYSYN_TRANSFORM_HELLO_TRANSFORM_ENABLED})
  set(OPENPHYSYN_LINKED_TRANSFORMS "${OPENPHYSYN_LINKED_TRANSFORMS} hello_transform")
endif()
if(${OPENPHYSYN_TRANSFORM_BUFFER_FANOUT_ENABLED})
  set(OPENPHYSYN_LINKED_TRANSFORMS "${OPENPHYSYN_LINKED_TRANSFORMS} buffer_fanout")
endif()
if(${OPENPHYSYN_TRANSFORM_GATE_CLONE_ENABLED})
  set(OPENPHYSYN_LINKED_TRANSFORMS "${OPENPHYSYN_LINKED_TRANSFORMS} gate_clone")
endif()
if(${OPENPHYSYN_TRANSFORM_PIN_SWAP_ENABLED})
  set(OPENPHYSYN_LINKED_TRANSFORMS "${OPENPHYSYN_LINKED_TRANSFORMS} pin_swap")
endif()
if(${OPENPHYSYN_TRANSFORM_CONSTANT_PROPAGATION_ENABLED})
  set(OPENPHYSYN_LINKED_TRANSFORMS "${OPENPHYSYN_LINKED_TRANSFORMS} constant_propagation")
endif()
if(${OPENPHYSYN_TRANSFORM_TIMING_BUFFER_ENABLED})
  set(OPENPHYSYN_LINKED_TRANSFORMS "${OPENPHYSYN_LINKED_TRANSFORMS} timing_buffer")
endif()
string(STRIP ${OPENPHYSYN_LINKED_TRANSFORMS} OPENPHYSYN_LINKED_TRANSFORMS)

string(STRIP ${OPENPHYSYN_LINKED_TRANSFORMS} OPENPHYSYN_LINKED_TRANSFORMS)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
include(CheckSymbolExists)
include(ConfigSafeGuards)
include(Colors)
include(CTest)
include(Doctest)
include(Documentation)
include(LTO)
include(Misc)
include(Warnings)
include(ClangFormat)
include(CppCheck)


# Check for LTO support.
find_lto(CXX)

#Add TCL Package
find_package(TCL REQUIRED)
if (NOT DEFINED TCL_LIB)
set(TCL_LIB ${TCL_LIBRARY})
endif()

#Add SWIG Package
find_package(ZLIB REQUIRED)




message(STATUS "OpenPhySyn version: ${PROJECT_VERSION}")

list(APPEND CMAKE_MODULE_PATH "${OPENSTA_HOME}/cmake")
include(GetGitRevisionDescription) # Utility available from OpenSTA
get_git_head_revision(GIT_REFSPEC OPENPHYSYN_GIT_SHA1)
message(STATUS "OpenPhySyn Git SHA: ${OPENPHYSYN_GIT_SHA1}")




set(PSN_WRAP  ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/Psn_lib_wrap.cc)
set(SWIG_SOURCE_COMPILE_FLAGS "-Wno-cast-qual -Wno-deprecated -Wno-unused-variable -Wno-unused-parameter")
set_source_files_properties(${PSN_WRAP} PROPERTIES COMPILE_FLAGS ${SWIG_SOURCE_COMPILE_FLAGS})

set(PSN_MAIN ${PSN_HOME}/app/PsnMain.cpp)
set(PSN_SOURCES
    ${PSN_HOME}/src/Psn.cpp
    ${PSN_HOME}/src/Exports.cpp
    ${PSN_HOME}/src/SteinerTree.cpp
    ${PSN_HOME}/src/PsnLogger.cpp
    ${PSN_HOME}/src/DatabaseHandler.cpp
    ${PSN_HOME}/src/PsnTransform.cpp
    ${PSN_HOME}/src/TransformHandler.cpp
    ${PSN_HOME}/src/TransformInfo.cpp
    ${PSN_HOME}/src/FileUtils.cpp
    ${PSN_HOME}/src/FilesystemLegacyHelpers.cpp
    ${PSN_HOME}/src/StringUtils.cpp
    ${PSN_HOME}/src/PsnGlobal.cpp
    ${PSN_HOME}/src/PsnException.cpp
    ${PSN_HOME}/src/PathPoint.cpp
    ${PSN_HOME}/src/MakeOpenPhySyn.cpp
    ${PSN_WRAP}
)




if (${OPENPHYSYN_TRANSFORM_HELLO_TRANSFORM_ENABLED})
add_definitions(-DOPENPHYSYN_TRANSFORM_HELLO_TRANSFORM_ENABLED)
endif()

if (${OPENPHYSYN_TRANSFORM_BUFFER_FANOUT_ENABLED})
add_definitions(-DOPENPHYSYN_TRANSFORM_BUFFER_FANOUT_ENABLED)
endif()

if (${OPENPHYSYN_TRANSFORM_GATE_CLONE_ENABLED})
add_definitions(-DOPENPHYSYN_TRANSFORM_GATE_CLONE_ENABLED)
endif()

if (${OPENPHYSYN_TRANSFORM_PIN_SWAP_ENABLED})
add_definitions(-DOPENPHYSYN_TRANSFORM_PIN_SWAP_ENABLED)
endif()

if (${OPENPHYSYN_TRANSFORM_CONSTANT_PROPAGATION_ENABLED})
add_definitions(-DOPENPHYSYN_TRANSFORM_CONSTANT_PROPAGATION_ENABLED)
endif()
if (${OPENPHYSYN_TRANSFORM_TIMING_BUFFER_ENABLED})
add_definitions(-DOPENPHYSYN_TRANSFORM_TIMING_BUFFER_ENABLED)
endif()



set(PSN_SWIG_FILES
  ${PSN_HOME}/src/Tcl/Swig/Psn.i
)
set(PSN_DEP_FILES
  ${PSN_SWIG_FILES}
  ${PSN_HOME}/src/Psn/Exports.cpp
  ${PSN_HOME}/src/Psn/Psn.cpp
  ${PSN_HOME}/src/Exports.hpp
  ${PSN_HOME}/src/Psn.hpp
)

set(PUBLIC_INCLUDE_DIRS
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/include/OpenPhySyn
  ${PROJECT_SOURCE_DIR}/src
)
set(PRIVATE_INCLUDE_DIRS
  ${PROJECT_SOURCE_DIR}/external/doctest
  ${PROJECT_SOURCE_DIR}/external/cxxopts/include
  ${PROJECT_SOURCE_DIR}/external/StandardTransforms
  ${PROJECT_SOURCE_DIR}/external/flute
  ${TCL_INCLUDE_PATH}
  ${FLUTE_HOME}/include
  ${OPENDB_HOME}/include
  ${DBSTA_HOME}/include
)



set(PUBLIC_LIBRARIES
${ZLIB_LIBRARIES}
${TCL_LIBRARY}
dl
OpenSTA
)

if(${OPENPHYSYN_TRANSFORM_HELLO_TRANSFORM_ENABLED})
  set(PUBLIC_LIBRARIES
    ${PUBLIC_LIBRARIES}
    hello_transform
  )
  endif()
  if(${OPENPHYSYN_TRANSFORM_BUFFER_FANOUT_ENABLED})
  set(PUBLIC_LIBRARIES
    ${PUBLIC_LIBRARIES}
    buffer_fanout
  )
endif()
if(${OPENPHYSYN_TRANSFORM_GATE_CLONE_ENABLED})
  set(PUBLIC_LIBRARIES
    ${PUBLIC_LIBRARIES}
    gate_clone
  )
  endif()
if(${OPENPHYSYN_TRANSFORM_PIN_SWAP_ENABLED})
  set(PUBLIC_LIBRARIES
    ${PUBLIC_LIBRARIES}
    pin_swap
  )
  endif()
if(${OPENPHYSYN_TRANSFORM_CONSTANT_PROPAGATION_ENABLED})
  set(PUBLIC_LIBRARIES
    ${PUBLIC_LIBRARIES}
    constant_propagation
  )
endif()
if(${OPENPHYSYN_TRANSFORM_TIMING_BUFFER_ENABLED})
  set(PUBLIC_LIBRARIES
    ${PUBLIC_LIBRARIES}
    timing_buffer
  )
endif()




set(PRIVATE_INCLUDE_DIRS
  ${PRIVATE_INCLUDE_DIRS}
  ${OPENROAD_HOME}/include
  ${OPENROAD_HOME}/src
  ${OPENROAD_HOME}/src/flute3
)



# Compile all sources into a library.
add_library(${LIBRARY_NAME} ${PSN_SOURCES})
set_target_properties(${LIBRARY_NAME} PROPERTIES COMPILE_FLAGS ${OPENPHYSYN_CMAKE_CXX_FLAGS})


target_include_directories(${LIBRARY_NAME} PUBLIC ${PUBLIC_INCLUDE_DIRS} PRIVATE ${PRIVATE_INCLUDE_DIRS})
target_link_libraries(${LIBRARY_NAME} PUBLIC ${PUBLIC_LIBRARIES})




include_directories (
  PUBLIC
  ${PUBLIC_INCLUDE_DIRS}
  PRIVATE
  ${PRIVATE_INCLUDE_DIRS}
)

target_compile_definitions(${LIBRARY_NAME} PRIVATE -DOPENROAD_OPENPHYSYN_LIBRARY_BUILD)


add_custom_command(OUTPUT ${PSN_WRAP}
  COMMAND ${SWIG_EXECUTABLE} ${SWIG_PSN_LIB_FLAGS} ${SWIG_FLAGS} -tcl8 -c++ -namespace -prefix psn -I${PROJECT_SOURCE_DIR}/src -o ${PSN_WRAP} ${PSN_SWIG_FILES}
  WORKING_DIRECTORY ${PSN_HOME}
  DEPENDS ${PSN_SWIG_FILES}
)

# Set up tests (see tests/CMakeLists.txt).
if (${OPENPHYSYN_UNIT_TESTS_ENABLED})
add_subdirectory(test)
endif()

set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES 
  ${PSN_WRAP}
)

if(OPENPHYSYN_STANDARD_TRANSFORMS_ENABLED)
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PSN_HOME}/external/StandardTransforms")
    include(Transforms)
endif()

target_link_libraries(${LIBRARY_NAME} PRIVATE
$<$<AND:$<CXX_COMPILER_ID:GNU>,$<VERSION_LESS:$<CXX_COMPILER_VERSION>,9.0>>:stdc++fs>)
