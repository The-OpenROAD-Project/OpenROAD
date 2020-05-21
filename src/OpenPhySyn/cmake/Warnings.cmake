# BSD 3-Clause License

# Copyright (c) 2019, SCALE Lab, Brown University
# All rights reserved.

# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:

# * Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.

# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.

# * Neither the name of the copyright holder nor the names of its
#   contributors may be used to endorse or promote products derived from
#   this software without specific prior written permission.

# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


function(target_set_warnings)
    if(NOT OPENPHYSYN_WARNINGS_SETTINGS_ENABLED)
        return()
    endif()
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
      set(WMSVC TRUE)
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
      set(WGCC TRUE)
    elseif ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
      set(WCLANG TRUE)
    endif()
    set(multiValueArgs ENABLE DISABLE AS_ERROR)
    cmake_parse_arguments(this "" "" "${multiValueArgs}" ${ARGN})
    list(FIND this_ENABLE "ALL" enable_all)
    list(FIND this_DISABLE "ALL" disable_all)
    list(FIND this_AS_ERROR "ALL" as_error_all)
    if(NOT ${enable_all} EQUAL -1)
      if(WMSVC)
        # Not all the warnings, but WAll is unusable when using libraries
        # Unless you'd like to support MSVC in the code with pragmas, this is probably the best option
        list(APPEND WarningFlags "/W4")
      elseif(WGCC)
        list(APPEND WarningFlags "-Wall" "-Wextra" "-Wpedantic")
      elseif(WCLANG)
        list(APPEND WarningFlags "-Wall" "-Weverything" "-Wpedantic")
      endif()
    elseif(NOT ${disable_all} EQUAL -1)
      set(SystemIncludes TRUE) # Treat includes as if coming from system
      if(WMSVC)
        list(APPEND WarningFlags "/w" "/W0")
      elseif(WGCC OR WCLANG)
        list(APPEND WarningFlags "-w")
      endif()
    endif()

    list(FIND this_DISABLE "Annoying" disable_annoying)
    if(NOT ${disable_annoying} EQUAL -1)
      if(WMSVC)
        # bounds-checked functions require to set __STDC_WANT_LIB_EXT1__ which we usually don't need/want
        list(APPEND WarningDefinitions -D_CRT_SECURE_NO_WARNINGS)
        # disable C4514 C4710 C4711... Those are useless to add most of the time
        #list(APPEND WarningFlags "/wd4514" "/wd4710" "/wd4711")
        #list(APPEND WarningFlags "/wd4365") #signed/unsigned mismatch
        #list(APPEND WarningFlags "/wd4668") # is not defined as a preprocessor macro, replacing with '0' for
      elseif(WGCC OR WCLANG)
        list(APPEND WarningFlags -Wno-switch-enum)
        if(WCLANG)
          list(APPEND WarningFlags -Wno-unknown-warning-option -Wno-padded -Wno-undef -Wno-reserved-id-macro -fcomment-block-commands=test,retval)
          if(NOT CMAKE_CXX_STANDARD EQUAL 98)
              list(APPEND WarningFlags -Wno-c++98-compat -Wno-c++98-compat-pedantic)
          endif()
          if ("${CMAKE_CXX_SIMULATE_ID}" STREQUAL "MSVC") # clang-cl has some VCC flags by default that it will not recognize...
              list(APPEND WarningFlags -Wno-unused-command-line-argument)
          endif()
        endif(WCLANG)
      endif()
    endif()

    if(NOT ${as_error_all} EQUAL -1)
      if(WMSVC)
        list(APPEND WarningFlags "/WX")
      elseif(WGCC OR WCLANG)
        list(APPEND WarningFlags "-Werror")
      endif()
    endif()
    foreach(target IN LISTS this_UNPARSED_ARGUMENTS)
      if(WarningFlags)
        target_compile_options(${target} PRIVATE ${WarningFlags})
      endif()
      if(WarningDefinitions)
        target_compile_definitions(${target} PRIVATE ${WarningDefinitions})
      endif()
      if(SystemIncludes)
        set_target_properties(${target} PROPERTIES
            INTERFACE_SYSTEM_INCLUDE_DIRECTORIES $<TARGET_PROPERTY:${target},INTERFACE_INCLUDE_DIRECTORIES>)
      endif()
    endforeach()
endfunction(target_set_warnings)
