# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2019-2025, The OpenROAD Authors

include("openroad")

find_package(LEMON NAMES LEMON lemon REQUIRED)

set(FLUTE_HOME ${PROJECT_SOURCE_DIR}/src/stt/src/flt)
set(PDR_HOME ${PROJECT_SOURCE_DIR}/src/stt/src/pdr)

set(POWV9_DAT ${FLUTE_HOME}/etc/POWV9.dat)
set(POST9_DAT ${FLUTE_HOME}/etc/POST9.dat)
set(POWV9_CPP ${CMAKE_CURRENT_BINARY_DIR}/POWV9.cpp)
set(POST9_CPP ${CMAKE_CURRENT_BINARY_DIR}/POST9.cpp)

add_custom_command(OUTPUT ${POWV9_CPP}
  COMMAND ${CMAKE_SOURCE_DIR}/etc/file_to_string.py 
  --inputs ${POWV9_DAT} 
  --output ${POWV9_CPP}
  --varname powv9 
  --namespace stt::flt 
  DEPENDS ${POWV9_DAT} ${PROJECT_SOURCE_DIR}/etc/file_to_string.py
)

add_custom_command(OUTPUT ${POST9_CPP}
  COMMAND ${PROJECT_SOURCE_DIR}/etc/file_to_string.py 
  --inputs ${POST9_DAT} 
  --output ${POST9_CPP} 
  --varname post9 
  --namespace stt::flt 
  DEPENDS ${POST9_DAT} ${PROJECT_SOURCE_DIR}/etc/file_to_string.py
  )

swig_lib(NAME      stt
         NAMESPACE stt
         I_FILE    src/SteinerTreeBuilder.i
         SCRIPTS   src/SteinerTreeBuilder.tcl
)

add_library(stt_lib
    src/SteinerTreeBuilder.cpp
    ${FLUTE_HOME}/flute.cpp
    ${POWV9_CPP}
    ${POST9_CPP}
    ${PDR_HOME}/src/pd.cpp
)

target_sources(stt
  PRIVATE
    src/MakeSteinerTreeBuilder.cpp
    src/LinesRenderer.cpp
)

target_include_directories(stt_lib
  PUBLIC
    include
  PRIVATE
    ${LEMON_INCLUDE_DIRS}
)

target_include_directories(stt
  PUBLIC
    include
  PRIVATE
    src
)

target_link_libraries(stt_lib
    utl_lib
    odb
)

target_link_libraries(stt
    stt_lib
    utl_lib
    gui
)

messages(
  TARGET stt
)

if (Python3_FOUND AND BUILD_PYTHON)
  swig_lib(NAME          stt_py
           NAMESPACE     stt
           LANGUAGE      python
           I_FILE        src/SteinerTreeBuilder-py.i
           SWIG_INCLUDES ${PROJECT_SOURCE_DIR}/include/stt
           SCRIPTS       ${CMAKE_CURRENT_BINARY_DIR}/stt_py.py
  )

  target_include_directories(stt_py
    PUBLIC
      include
  )

  target_link_libraries(stt_py
    PUBLIC
      stt
  )

endif()

add_subdirectory(test)
