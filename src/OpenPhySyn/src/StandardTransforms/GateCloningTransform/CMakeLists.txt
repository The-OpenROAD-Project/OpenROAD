project(OpenPhySynGateCloningTransform VERSION 1.0.0 LANGUAGES CXX)

set(TRANSFORM_NAME gate_clone)

set(PSN_HOME "/usr/include/OpenPhySyn" CACHE FILEPATH "The path to OpenPhySyn includes")

IF(NOT EXISTS ${PSN_HOME})
message( FATAL_ERROR "Cannot find OpenPhySyn headers, set PSN_HOME option." )
endif()

if(NOT DEFINED OPENDB_HOME)
  set(OPENDB_HOME ${PSN_HOME}/external/OpenDB CACHE FILEPATH "The path to OpenDB includes")
endif()

if(NOT DEFINED OPENSTA_HOME)
  set(OPENSTA_HOME ${PSN_HOME}/external/OpenSTA  CACHE FILEPATH "The path to OpenSTA" )
endif()

IF(NOT EXISTS ${OPENDB_HOME})
message( FATAL_ERROR "${OPENDB_HOME} -- ${PSN_HOME} Cannot find OpenDB headers, set OPENDB_HOME option." )
endif()
IF(NOT EXISTS ${OPENSTA_HOME})
message( FATAL_ERROR "Cannot find OpenSTA headers, set OPENSTA_HOME option." )
endif()

set(PSN_LIB_PATH "" CACHE STRING "Set OpenPhySyn built library path" 0)

if(NOT PSN_LIB_PATH STREQUAL 0)
link_directories(${PSN_LIB_PATH})
endif()

if(${OPENPHYSYN_TRANSFORM_AUTO_LINK_TRANSFORMS})
add_library(${TRANSFORM_NAME}  src/GateCloningTransform.cpp)
target_compile_definitions(${TRANSFORM_NAME} INTERFACE OPENPHYSYN_TRANSFORM_BUILD)
else()
add_library(${TRANSFORM_NAME} SHARED src/GateCloningTransform.cpp)
endif()

target_include_directories(${TRANSFORM_NAME} PUBLIC
  src/
  ${PSN_HOME}/include/
  ${PSN_HOME}/external/spdlog/include
  ${OPENDB_HOME}/include
  ${OPENDB_HOME}/src
  ${OPENDB_HOME}/src/db
  ${OPENSTA_HOME}/..
)

target_link_libraries(${TRANSFORM_NAME}
PUBLIC
${OPENPHYSYB_TRANSFORMS_LINKING_LIBRARY}
)

if (DEFINED "TRANSFORMS_BUILD_DIR")
set_target_properties(${TRANSFORM_NAME}
    PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${TRANSFORMS_BUILD_DIR}"
)
endif()

target_compile_features(${TRANSFORM_NAME} PRIVATE cxx_range_for cxx_auto_type)
target_compile_options(${TRANSFORM_NAME} PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wall>)
set_property(TARGET ${TRANSFORM_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)


set(PSN_TRANSFORM_INSTALL_PATH "$ENV{HOME}/.OpenPhySyn/transforms" CACHE STRING "Default path for transforms installation")


install(
  TARGETS ${TRANSFORM_NAME}
  DESTINATION ${PSN_TRANSFORM_INSTALL_PATH}
)