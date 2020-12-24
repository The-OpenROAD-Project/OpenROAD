# LEF PARSER CMAKE Compile Settings
project(LEF_PARSER 
  LANGUAGES CXX 
)

set(LEFLIB_HOME ${CMAKE_CURRENT_SOURCE_DIR})
message(STATUS "leflib_home: ${LEFLIB_HOME}")

# BISON and ZLIB are required to compile LEF/LEF Parsers.
find_package(BISON)
find_package(ZLIB)

############################################################
# Create a library for LEF
############################################################
set( LEFLIB_LEF_SRC
  lef/crypt.cpp
  lef/lefiArray.cpp
  lef/lefiCrossTalk.cpp
  lef/lefiDebug.cpp
  lef/lefiEncryptInt.cpp
  lef/lefiLayer.cpp
  lef/lefiMacro.cpp
  lef/lefiMisc.cpp
  lef/lefiNonDefault.cpp
  lef/lefiProp.cpp
  lef/lefiPropType.cpp
  lef/lefiTBExt.cpp
  lef/lefiUnits.cpp
  lef/lefiVia.cpp
  lef/lefiViaRule.cpp
  lef/lef_keywords.cpp
  lef/lefrCallbacks.cpp
  lef/lefrData.cpp
  lef/lefrReader.cpp
  lef/lefrSettings.cpp
  lef/lef.tab.cpp
  lef/lefwWriterCalls.cpp
  lef/lefwWriter.cpp
  )

set( LEFLIB_LEFZLIB_SRC 
  lefzlib/lefzlib.cpp
  )

set (LEFLIB_HEADERS
  lef/lef.tab.h
  lef/lex.h
  lef/crypt.hpp
  lef/lefiArray.hpp
  lef/lefiCrossTalk.hpp
  lef/lefiDebug.hpp
  lef/lefiDefs.hpp
  lef/lefiEncryptInt.hpp
  lef/lefiKRDefs.hpp
  lef/lefiLayer.hpp
  lef/lefiMacro.hpp
  lef/lefiMisc.hpp
  lef/lefiNonDefault.hpp
  lef/lefiProp.hpp
  lef/lefiPropType.hpp
  lef/lefiUnits.hpp
  lef/lefiUser.hpp
  lef/lefiUtil.hpp
  lef/lefiVia.hpp
  lef/lefiViaRule.hpp
  lef/lefrCallBacks.hpp
  lef/lefrData.hpp
  lef/lefrReader.hpp
  lef/lefrSettings.hpp
  lef/lefwWriterCalls.hpp
  lef/lefwWriter.hpp

  lefzlib/lefzlib.hpp
)

# BISON dependencies
add_custom_command( 
  OUTPUT ${LEFLIB_HOME}/lef/lef.tab.cpp
  COMMAND ${BISON_EXECUTABLE} -v -plefyy -d ${LEFLIB_HOME}/lef/lef.y
  COMMAND mv lef.tab.h ${LEFLIB_HOME}/lef/lef.tab.h
  COMMAND mv lef.tab.c ${LEFLIB_HOME}/lef/lef.tab.cpp
  DEPENDS ${LEFLIB_HOME}/lef/lef.y
)
add_custom_target(LefBisonTarget ALL DEPENDS lef/lef.tab.cpp)


add_library( lef ${LEFLIB_LEF_SRC} )
add_library( lefzlib ${LEFLIB_LEFZLIB_SRC} )

set_target_properties( lef
  PROPERTIES
  ARCHIVE_OUTPUT_DIRECTORY ${LEFLIB_HOME}/lib
  LIBRARY_OUTPUT_DIRECTORY ${LEFLIB_HOME}/lib
  RUNTIME_OUTPUT_DIRECTORY ${LEFLIB_HOME}/lib
  # python regression requirement (gag me) -cherry
  POSITION_INDEPENDENT_CODE ON
)

set_target_properties( lefzlib
  PROPERTIES
  ARCHIVE_OUTPUT_DIRECTORY ${LEFLIB_HOME}/lib
  LIBRARY_OUTPUT_DIRECTORY ${LEFLIB_HOME}/lib
  RUNTIME_OUTPUT_DIRECTORY ${LEFLIB_HOME}/lib
  # python regression requirement (gag me) -cherry
  POSITION_INDEPENDENT_CODE ON
)

# include directory settings
target_include_directories( lef
  PUBLIC
  lef
  )

target_include_directories( lefzlib
  PUBLIC
  lef
  lefzlib
  )

add_dependencies( lef LefBisonTarget )
add_dependencies( lefzlib lef )

######################################################
# install

install(FILES ${LEFLIB_HEADERS} DESTINATION include)
install(TARGETS lef lefzlib DESTINATION lib)
