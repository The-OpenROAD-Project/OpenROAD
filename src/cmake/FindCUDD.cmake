# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

include(FindPackageHandleStandardArgs)

set(_CUDD_HINTS)

foreach(_cudd_root_var CUDD_ROOT cudd_ROOT)
  if(DEFINED ${_cudd_root_var} AND NOT "${${_cudd_root_var}}" STREQUAL "")
    list(APPEND _CUDD_HINTS "${${_cudd_root_var}}")
  endif()
endforeach()

foreach(_cudd_root_env CUDD_ROOT cudd_ROOT)
  if(DEFINED ENV{${_cudd_root_env}} AND NOT "$ENV{${_cudd_root_env}}" STREQUAL "")
    list(APPEND _CUDD_HINTS "$ENV{${_cudd_root_env}}")
  endif()
endforeach()

if(CUDD_LIB AND NOT CUDD_LIBRARY)
  set(CUDD_LIBRARY "${CUDD_LIB}" CACHE FILEPATH "Path to the CUDD library")
endif()

if(CUDD_INCLUDE AND NOT CUDD_INCLUDE_DIR)
  if(IS_DIRECTORY "${CUDD_INCLUDE}")
    set(CUDD_INCLUDE_DIR "${CUDD_INCLUDE}" CACHE PATH "Path to the CUDD include directory")
  else()
    get_filename_component(_cudd_include_dir "${CUDD_INCLUDE}" DIRECTORY)
    set(CUDD_INCLUDE_DIR "${_cudd_include_dir}" CACHE PATH "Path to the CUDD include directory")
  endif()
endif()

if(CUDD_LIBRARY)
  get_filename_component(_cudd_lib_dir "${CUDD_LIBRARY}" DIRECTORY)
  get_filename_component(_cudd_prefix "${_cudd_lib_dir}" DIRECTORY)
  list(APPEND _CUDD_HINTS "${_cudd_prefix}")
endif()

find_library(CUDD_LIBRARY
  NAMES cudd
  HINTS ${_CUDD_HINTS}
  PATH_SUFFIXES lib lib64
)

find_path(CUDD_INCLUDE_DIR
  NAMES cudd.h
  HINTS ${_CUDD_HINTS}
  PATH_SUFFIXES include include/cudd
)

find_package_handle_standard_args(CUDD
  REQUIRED_VARS CUDD_LIBRARY CUDD_INCLUDE_DIR
)

if(CUDD_FOUND)
  set(CUDD_LIB "${CUDD_LIBRARY}" CACHE FILEPATH "Path to the CUDD library")
  set(CUDD_INCLUDE "${CUDD_INCLUDE_DIR}" CACHE PATH "Path to the CUDD include directory")

  if(NOT TARGET CUDD::CUDD)
    add_library(CUDD::CUDD UNKNOWN IMPORTED)
    set_target_properties(CUDD::CUDD PROPERTIES
      IMPORTED_LOCATION "${CUDD_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${CUDD_INCLUDE_DIR}"
    )
  endif()
endif()

mark_as_advanced(CUDD_LIBRARY CUDD_INCLUDE_DIR CUDD_LIB CUDD_INCLUDE)
