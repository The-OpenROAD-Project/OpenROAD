include(FindPackageHandleStandardArgs)

if( CMAKE_VTUNE_HOME )
  set( VTUNE_HOME ${CMAKE_VTUNE_HOME} )
elseif( DEFINED ENV{CMAKE_VTUNE_HOME} )
  set( VTUNE_HOME $ENV{CMAKE_VTUNE_HOME} )
else()
  set( VTUNE_HOME /home/tool/intel/vtune_amplifier)
endif()

          
find_path(VTune_INCLUDE_DIRS ittnotify.h
  PATHS ${VTUNE_HOME}
  PATH_SUFFIXES include)

find_library(VTune_LIBRARIES ittnotify
  HINTS "${VTune_INCLUDE_DIRS}/.."
  PATHS ${VTUNE_HOME}
  PATH_SUFFIXES lib64)

find_package_handle_standard_args(
    VTune DEFAULT_MSG VTune_LIBRARIES VTune_INCLUDE_DIRS)

if( VTune_FOUND AND NOT TARGET VTune::VTune )
  add_library(VTune::VTune UNKNOWN IMPORTED)
  set_target_properties(VTune::VTune PROPERTIES
    IMPORTED_LOCATION "${VTune_LIBRARIES}"
    INTERFACE_INCLUDE_DIRECTORIES "${VTune_INCLUDE_DIRS}"
  )

  mark_as_advanced(
    VTune_INCLUDE_DIR
    VTune_LIBRARIES
  )

  target_link_libraries( VTune::VTune
    INTERFACE
    ${CMAKE_DL_LIBS}
  )
endif()
