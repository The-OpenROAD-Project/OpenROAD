# BISON and ZLIB are required to compile LEF/LEF Parsers.
find_package(BISON REQUIRED)
find_package(ZLIB)

bison_target(lef_parser
             ${CMAKE_CURRENT_SOURCE_DIR}/lef/lef.y
             ${CMAKE_CURRENT_BINARY_DIR}/lef_parser.cpp
             COMPILE_FLAGS "--name-prefix=lefyy"
)

############################################################
# Create a library for LEF
############################################################
add_library(lef
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
  lef/lefwWriterCalls.cpp
  lef/lefwWriter.cpp
  ${BISON_lef_parser_OUTPUT_SOURCE}
)

add_library(lefzlib
  lefzlib/lefzlib.cpp
)

set_target_properties(lef lefzlib
  PROPERTIES
    # python regression requirement
    POSITION_INDEPENDENT_CODE ON
)

# include directory settings
target_include_directories(lef
  PUBLIC
    lef
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR} # for lef_parser.hpp
)

target_include_directories(lefzlib
  PUBLIC
    lef
    lefzlib
)
target_link_libraries(lefzlib
    PUBLIC
        ZLIB::ZLIB
)

target_compile_options(lef
  PRIVATE
    -Wno-class-memaccess
)
