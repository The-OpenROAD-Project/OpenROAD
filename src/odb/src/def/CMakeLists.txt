# DEF PARSER CMAKE Compile Settings
project(DEF_PARSER 
  LANGUAGES CXX 
)

# BISON and ZLIB are required to compile DEF/LEF Parsers.
find_package(BISON REQUIRED)
find_package(ZLIB)

bison_target(def_parser
             ${CMAKE_CURRENT_SOURCE_DIR}/def/def.y
             ${CMAKE_CURRENT_BINARY_DIR}/def_parser.cpp
             COMPILE_FLAGS "--name-prefix=defyy"
)

add_library(def
  def/defiAlias.cpp
  def/defiAssertion.cpp
  def/defiBlockage.cpp
  def/defiComponent.cpp
  def/defiDebug.cpp
  def/defiFill.cpp
  def/defiFPC.cpp
  def/defiGroup.cpp
  def/defiIOTiming.cpp
  def/defiMisc.cpp
  def/defiNet.cpp
  def/defiNonDefault.cpp
  def/defiPartition.cpp
  def/defiPath.cpp
  def/defiPinCap.cpp
  def/defiPinProp.cpp
  def/defiProp.cpp
  def/defiPropType.cpp
  def/defiRegion.cpp
  def/defiRowTrack.cpp
  def/defiScanchain.cpp
  def/defiSite.cpp
  def/defiSlot.cpp
  def/defiTimingDisable.cpp
  def/defiUtil.cpp
  def/defiVia.cpp
  def/def_keywords.cpp
  def/defrCallbacks.cpp
  def/defrData.cpp
  def/defrReader.cpp
  def/defrSettings.cpp
  def/defwWriterCalls.cpp
  def/defwWriter.cpp
  ${BISON_def_parser_OUTPUT_SOURCE}
)

add_library(defzlib
  defzlib/defzlib.cpp
)

set_target_properties(def defzlib
  PROPERTIES
    # python requirement
    POSITION_INDEPENDENT_CODE ON
)

# include directory settings
target_include_directories(def
  PUBLIC
    def
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR} # for def_parser.hpp
)

target_include_directories(defzlib
  PUBLIC
    def
    defzlib
)
target_link_libraries(defzlib
    PUBLIC
        ZLIB::ZLIB
)
