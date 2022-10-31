###############################################################################
##
## BSD 3-Clause License
##
## Copyright (c) 2019, Parallax Software, Inc.
## All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions are met:
##
## * Redistributions of source code must retain the above copyright notice, this
##   list of conditions and the following disclaimer.
##
## * Redistributions in binary form must reproduce the above copyright notice,
##   this list of conditions and the following disclaimer in the documentation
##   and#or other materials provided with the distribution.
##
## * Neither the name of the copyright holder nor the names of its
##   contributors may be used to endorse or promote products derived from
##   this software without specific prior written permission.
##
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
## AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
## IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
## ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
## LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
## CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
## SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
## INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
## CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
## ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
## POSSIBILITY OF SUCH DAMAGE.
##
###############################################################################

################################################################
#
# Locate TCL library.
#
# Note that the cmake findTcl module is hopeless for OSX
# because there doesn't appear to be a way to override
# searching OSX system directories before unix directories.

set(TCL_POSSIBLE_NAMES tcl87 tcl8.7
  tcl86 tcl8.6
  tcl85 tcl8.5
  tcl84 tcl8.4
  tcl83 tcl8.3
  tcl82 tcl8.2
  )

# tcl lib path guesses.
if (NOT TCL_LIB_PATHS)
  if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(TCL_LIB_PATHS /usr/local/lib
      /opt/homebrew/opt/tcl-tk/lib
      /usr/local/opt/tcl-tk/lib
      )
    set(TCL_NO_DEFAULT_PATH TRUE)
  elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(TCL_LIB_PATHS /usr/lib /usr/local/lib)
    set(TCL_NO_DEFAULT_PATH FALSE)
  endif()
endif()

if (NOT TCL_LIBRARY)
  # bagbiter cmake doesn't have a way to pass NO_DEFAULT_PATH as a parameter.
  if (TCL_NO_DEFAULT_PATH)
    find_library(TCL_LIBRARY
      NAMES tcl ${TCL_POSSIBLE_NAMES}
      PATHS ${TCL_LIB_PATHS}
      NO_DEFAULT_PATH
      )
  else()
    find_library(TCL_LIBRARY
      NAMES tcl ${TCL_POSSIBLE_NAMES}
      PATHS ${TCL_LIB_PATHS}
      )
  endif()
endif()
message(STATUS "TCL library: ${TCL_LIBRARY}")

get_filename_component(TCL_LIB_DIR "${TCL_LIBRARY}" PATH)
get_filename_component(TCL_LIB_PARENT1 "${TCL_LIB_DIR}" PATH)
get_filename_component(TCL_LIB_PARENT2 "${TCL_LIB_PARENT1}" PATH)

# Locate tcl.h
if (NOT TCL_HEADER)
  find_file(TCL_HEADER tcl.h
    PATHS ${TCL_LIB_PARENT1} ${TCL_LIB_PARENT2}
    PATH_SUFFIXES include include/tcl
    NO_DEFAULT_PATH
    )
endif()
message(STATUS "TCL header: ${TCL_HEADER}")

if (TCL_HEADER)
  get_filename_component(TCL_INCLUDE_PATH "${TCL_HEADER}" PATH)
endif()
