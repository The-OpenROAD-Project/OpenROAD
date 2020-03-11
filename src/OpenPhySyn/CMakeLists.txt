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
option(OPENPHYSYN_TRANSFORM_AUTO_LINK_TRANSFORMS "Automatically link built transforms" ON)

set(LIBRARY_NAME ${PROJECT_NAME})  # Default name for the library built from src/*.cpp (change if you wish)
# string(TOLOWER ${PROJECT_NAME} LIBRARY_NAME)


set(OPENPHYSYN_LINKED_TRANSFORMS "")
set(OPENPHYSYN_TRANSFORMS_LINKING_LIBRARY ${LIBRARY_NAME})


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -Wno-error -Wno-deprecated-copy  ")
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -rdynamic -Wl,-export_dynamic -Wno-unused-command-line-argument")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -rdynamic")
endif()
set(CMAKE_POSITION_INDEPENDENT_CODE ON)


# Tool build configuration
option(OPENROAD_BUILD "Indicates if the tool is being built as a module in the top level application" OFF)
set(OPENPHYSYN_DATABASE_HANDLER "OpenSTA" CACHE STRING "Specify the database handler type, can be OpenDB or OpenSTA")


add_definitions(-DOPENROAD_BUILD)
if(${OPENPHYSYN_TRANSFORM_AUTO_LINK_TRANSFORMS})
add_definitions(-DOPENPHYSYN_AUTO_LINK)
set(OPENPHYSYN_TRANSFORMS_LINKING_LIBRARY "")
endif()

set(SWIG_FLAGS "-w467,72,503,509")
set(SWIG_PSN_LIB_FLAGS "")
set(SWIG_FLAGS "-DOPENROAD_BUILD -DSWIG_DIRECTOR_STATIC ${SWIG_FLAGS}")
set(SWIG_PSN_LIB_FLAGS "-DOPENROAD_OPENPHYSYN_LIBRARY_BUILD")


set(PSN_HOME
  ${PROJECT_SOURCE_DIR}
)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PSN_HOME}/cmake")


# Default to bulding optimnized/release executable.
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RELEASE)
endif()

if(${OPENPHYSYN_TRANSFORM_AUTO_LINK_TRANSFORMS})
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
  string(STRIP ${OPENPHYSYN_LINKED_TRANSFORMS} OPENPHYSYN_LINKED_TRANSFORMS)
endif()

string(STRIP ${OPENPHYSYN_LINKED_TRANSFORMS} OPENPHYSYN_LINKED_TRANSFORMS)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
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


# Build spdlok
if(NOT DEFINED SPD_LOG_HOME)
set(SPD_LOG_HOME
  ${PROJECT_SOURCE_DIR}/external/spdlog
)
endif()


# Add spdlog
add_subdirectory(external/spdlog)



message(STATUS "OpenPhySyn version: ${PROJECT_VERSION}")

list(APPEND CMAKE_MODULE_PATH "${OPENSTA_HOME}/cmake")
include(GetGitRevisionDescription) # Utility available from OpenSTA
get_git_head_revision(GIT_REFSPEC OPENPHYSYN_GIT_SHA1)
message(STATUS "OpenPhySyn Git SHA: ${OPENPHYSYN_GIT_SHA1}")




set(PSN_WRAP  ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/Psn_wrap.cc)
set(PSN_LIBRARY_WRAP  ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/Psn_lib_wrap.cc)
set(SWIG_SOURCE_COMPILE_FLAGS "-Wno-cast-qual -Wno-deprecated-copy -Wno-unused-variable -Wno-unused-parameter -Wno-deprecated-copy")
set_source_files_properties(${PSN_WRAP} PROPERTIES COMPILE_FLAGS ${SWIG_SOURCE_COMPILE_FLAGS})
set_source_files_properties(${PSN_LIBRARY_WRAP} PROPERTIES COMPILE_FLAGS ${SWIG_SOURCE_COMPILE_FLAGS})

set(PSN_MAIN ${PSN_HOME}/app/PsnMain.cpp)
set(PSN_SOURCES_NO_MAIN_NO_WRAP
    ${PSN_HOME}/src/Psn.cpp
    ${PSN_HOME}/src/Exports.cpp
    ${PSN_HOME}/src/DesignSettings.cpp
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
)

set(PSN_OPENROAD_LIBRARY_FILE
  ${PSN_HOME}/src/MakeOpenPhySyn.cpp
)



set(PSN_SOURCES_NO_MAIN
    ${PSN_SOURCES_NO_MAIN_NO_WRAP}
    ${PSN_WRAP}
)
set(PSN_SOURCES 
    ${PSN_SOURCES_NO_MAIN}
    ${PSN_MAIN}
)

set(PSN_TESTFILES        # All .cpp files in tests/
    ${PROJECT_SOURCE_DIR}/tests/doctest/SteinerTree.cpp
    ${PROJECT_SOURCE_DIR}/tests/doctest/HelloTransform.cpp
    ${PROJECT_SOURCE_DIR}/tests/doctest/Sta.cpp
    ${PROJECT_SOURCE_DIR}/tests/doctest/TestMain.cpp
    ${PROJECT_SOURCE_DIR}/tests/doctest/Readers.cpp
)
if (${OPENPHYSYN_TRANSFORM_HELLO_TRANSFORM_ENABLED})
set(PSN_TESTFILES
  ${PSN_TESTFILES}
  ${PROJECT_SOURCE_DIR}/tests/doctest/HelloTransform.cpp
)
add_definitions(-DOPENPHYSYN_TRANSFORM_HELLO_TRANSFORM_ENABLED)
endif()
if (${OPENPHYSYN_TRANSFORM_BUFFER_FANOUT_ENABLED})
set(PSN_TESTFILES
  ${PSN_TESTFILES}
  ${PROJECT_SOURCE_DIR}/tests/doctest/BufferFanout.cpp
  ${PROJECT_SOURCE_DIR}/tests/doctest/BufferTopPort.cpp
)
add_definitions(-DOPENPHYSYN_TRANSFORM_BUFFER_FANOUT_ENABLED)
endif()
if (${OPENPHYSYN_TRANSFORM_GATE_CLONE_ENABLED})
set(PSN_TESTFILES
  ${PSN_TESTFILES}
  ${PROJECT_SOURCE_DIR}/tests/doctest/GateClone.cpp
)
add_definitions(-DOPENPHYSYN_TRANSFORM_GATE_CLONE_ENABLED)
endif()
if (${OPENPHYSYN_TRANSFORM_PIN_SWAP_ENABLED})
set(PSN_TESTFILES
  ${PSN_TESTFILES}
  ${PROJECT_SOURCE_DIR}/tests/doctest/PinSwap.cpp
  ${PROJECT_SOURCE_DIR}/tests/doctest/PinSwapPower.cpp
)
add_definitions(-DOPENPHYSYN_TRANSFORM_PIN_SWAP_ENABLED)
endif()
if (${OPENPHYSYN_TRANSFORM_CONSTANT_PROPAGATION_ENABLED})
set(PSN_TESTFILES
  ${PSN_TESTFILES}
  ${PROJECT_SOURCE_DIR}/tests/doctest/SimpleConstantPropagation.cpp
  ${PROJECT_SOURCE_DIR}/tests/doctest/SimpleConstantPropagationInversion.cpp
  ${PROJECT_SOURCE_DIR}/tests/doctest/ComplexConstantPropagation.cpp
)
add_definitions(-DOPENPHYSYN_TRANSFORM_CONSTANT_PROPAGATION_ENABLED)
endif()


set(PSN_SOURCES
    ${PSN_SOURCES}
    ${DBSTA_TCL_INIT}
)

set(PSN_TESTFILES
    ${PSN_TESTFILES}
    ${DBSTA_TCL_INIT}
)
add_custom_target(
  opensta-tcl-init
  DEPENDS ${DBSTA_TCL_INIT}
)




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
  ${PROJECT_SOURCE_DIR}/external/spdlog
  ${PROJECT_SOURCE_DIR}/external/cxxopts/include
  ${PROJECT_SOURCE_DIR}/external/StandardTransforms
  ${PROJECT_SOURCE_DIR}/external/flute
  ${TCL_INCLUDE_PATH}
  ${SPD_LOG_HOME}/include
  ${FLUTE_HOME}/include
  ${OPENDB_HOME}/include
  ${DBSTA_HOME}/include
)



set(PUBLIC_LIBRARIES
${ZLIB_LIBRARIES}
${TCL_LIBRARY}
dl
)
if(${OPENPHYSYN_TRANSFORM_AUTO_LINK_TRANSFORMS})
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
endif()


CHECK_SYMBOL_EXISTS(std::filesystem::path::preferred_separator filesystem cxx17fs)

if(cxx17fs AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.9.2)
set(PUBLIC_LIBRARIES
  ${PUBLIC_LIBRARIES}
  stdc++fs
)
endif()

# Stand-alone executable specific libraries
set(PUBLIC_EXEC_LIBRARIES
)

if (${OPENPHYSYN_TF_ENABLED})
set(PUBLIC_LIBRARIES
  ${PUBLIC_LIBRARIES}
  Cpp-Taskflow
)
endif()


set(PRIVATE_INCLUDE_DIRS
  ${PRIVATE_INCLUDE_DIRS}
  ${OPENROAD_HOME}/include
  ${OPENROAD_HOME}/src
  ${OPENROAD_HOME}/src/flute3
  ${OPENDB_HOME}/include
  ${OPENSTA_HOME}/app
  ${OPENSTA_HOME}/dcalc
  ${OPENSTA_HOME}/graph
  ${OPENSTA_HOME}/liberty
  ${OPENSTA_HOME}/network
  ${OPENSTA_HOME}/parasitics
  ${OPENSTA_HOME}/sdc
  ${OPENSTA_HOME}/sdf
  ${OPENSTA_HOME}/search
  ${OPENSTA_HOME}/util
  ${OPENSTA_HOME}/verilog
  ${OPENSTA_HOME}/${CUDD_INCLUDE}
  flute3
)

set(PUBLIC_EXEC_LIBRARIES
opendb
dbSta
OpenSTA
flute
)


# Compile all sources into a library.
add_library(${LIBRARY_NAME} ${PSN_SOURCES_NO_MAIN_NO_WRAP} ${PSN_LIBRARY_WRAP} ${PSN_OPENROAD_LIBRARY_FILE})

target_include_directories(${LIBRARY_NAME} PUBLIC ${PUBLIC_INCLUDE_DIRS} PRIVATE ${PRIVATE_INCLUDE_DIRS})
target_link_libraries(${LIBRARY_NAME} PUBLIC ${PUBLIC_LIBRARIES}) 



include_directories (
  PUBLIC
  ${PUBLIC_INCLUDE_DIRS}
  PRIVATE
  ${PRIVATE_INCLUDE_DIRS}
)

target_compile_definitions(${LIBRARY_NAME} PRIVATE -DOPENROAD_OPENPHYSYN_LIBRARY_BUILD)

set_property(TARGET ${LIBRARY_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)


add_custom_command(OUTPUT ${PSN_WRAP}
  COMMAND ${SWIG_EXECUTABLE} ${SWIG_FLAGS} -tcl8 -c++ -namespace -prefix psn -I${PROJECT_SOURCE_DIR}/src -o ${PSN_WRAP} ${PSN_SWIG_FILES}
  WORKING_DIRECTORY ${PSN_HOME}
  DEPENDS ${PSN_SWIG_FILES}
)
add_custom_command(OUTPUT ${PSN_LIBRARY_WRAP}
  COMMAND ${SWIG_EXECUTABLE} ${SWIG_PSN_LIB_FLAGS} ${SWIG_FLAGS} -tcl8 -c++ -namespace -prefix psn -I${PROJECT_SOURCE_DIR}/src -o ${PSN_LIBRARY_WRAP} ${PSN_SWIG_FILES}
  WORKING_DIRECTORY ${PSN_HOME}
  DEPENDS ${PSN_SWIG_FILES}
)

# Set up tests (see tests/CMakeLists.txt).
if (${OPENPHYSYN_UNIT_TESTS_ENABLED})
add_subdirectory(test)
endif()

set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES 
  ${PSN_WRAP}
  ${PSN_LIBRARY_WRAP}
)

if(OPENPHYSYN_STANDARD_TRANSFORMS_ENABLED)
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PSN_HOME}/external/StandardTransforms")
    include(Transforms)
endif()
