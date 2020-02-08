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


# Usage :
#
# Variable : OPENPHYSYN_LTO_ENABLED | Enable or disable LTO support for this build
#
# find_lto(lang)
# - lang is C or CXX (the language to test LTO for)
# - call it after project() so that the compiler is already detected
#
# This will check for LTO support and create a target_enable_lto(target [debug,optimized,general]) macro.
# The 2nd parameter has the same meaning as in target_link_libraries, and is used to enable LTO only for those build configurations
# 'debug' is by default the Debug configuration, and 'optimized' all the other configurations
#
# if OPENPHYSYN_LTO_ENABLED is set to false, an empty macro will be generated
#
# Then to enable LTO for your target use
#
#       target_enable_lto(mytarget general)
#
# It is however recommended to use it only for non debug builds the following way :
#
#       target_enable_lto(mytarget optimized)
#
# Note : For CMake versions < 3.9, target_link_library is used in it's non plain version.
#        You will need to specify PUBLIC/PRIVATE/INTERFACE to all your other target_link_library calls for the target
#
# WARNING for cmake versions older than 3.9 :
# This module will override CMAKE_AR CMAKE_RANLIB and CMAKE_NM by the gcc versions if found when building with gcc


# License:
#
# Copyright (C) 2016 Lectem <lectem@gmail.com>
#
# Permission is hereby granted, free of charge, to any person
# obtaining a copy of this software and associated documentation files
# (the 'Software') deal in the Software without restriction,
# including without limitation the rights to use, copy, modify, merge,
# publish, distribute, sublicense, and/or sell copies of the Software,
# and to permit persons to whom the Software is furnished to do so,
# subject to the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
# BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
# ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.


macro(find_lto lang)
    if(OPENPHYSYN_LTO_ENABLED AND NOT LTO_${lang}_CHECKED)

      #LTO support was added for clang/gcc in 3.9
      if(${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} VERSION_LESS 3.9)
          cmake_policy(SET CMP0054 NEW)
		  message(STATUS "Checking for LTO Compatibility")
          # Since GCC 4.9 we need to use gcc-ar / gcc-ranlib / gcc-nm
          if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
              if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND NOT CMAKE_GCC_AR OR NOT CMAKE_GCC_RANLIB OR NOT CMAKE_GCC_NM)
                  find_program(CMAKE_GCC_AR NAMES
                    "${_CMAKE_TOOLCHAIN_PREFIX}gcc-ar"
                    "${_CMAKE_TOOLCHAIN_PREFIX}gcc-ar-${_version}"
                    DOC "gcc provided wrapper for ar which adds the --plugin option"
                  )
                  find_program(CMAKE_GCC_RANLIB NAMES
                    "${_CMAKE_TOOLCHAIN_PREFIX}gcc-ranlib"
                    "${_CMAKE_TOOLCHAIN_PREFIX}gcc-ranlib-${_version}"
                    DOC "gcc provided wrapper for ranlib which adds the --plugin option"
                  )
                  # Not needed, but at least stay coherent
                  find_program(CMAKE_GCC_NM NAMES
                    "${_CMAKE_TOOLCHAIN_PREFIX}gcc-nm"
                    "${_CMAKE_TOOLCHAIN_PREFIX}gcc-nm-${_version}"
                    DOC "gcc provided wrapper for nm which adds the --plugin option"
                  )
                  mark_as_advanced(CMAKE_GCC_AR CMAKE_GCC_RANLIB CMAKE_GCC_NM)
                  set(CMAKE_LTO_AR ${CMAKE_GCC_AR})
                  set(CMAKE_LTO_RANLIB ${CMAKE_GCC_RANLIB})
                  set(CMAKE_LTO_NM ${CMAKE_GCC_NM})
              endif()
              if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
                  set(CMAKE_LTO_AR ${CMAKE_AR})
                  set(CMAKE_LTO_RANLIB ${CMAKE_RANLIB})
                  set(CMAKE_LTO_NM ${CMAKE_NM})
              endif()

              if(CMAKE_LTO_AR AND CMAKE_LTO_RANLIB)
                set(__lto_flags -flto)

                if(NOT CMAKE_${lang}_COMPILER_VERSION VERSION_LESS 4.7)
                  list(APPEND __lto_flags -fno-fat-lto-objects)
                endif()

                if(NOT DEFINED CMAKE_${lang}_PASSED_LTO_TEST)
                  set(__output_dir "${CMAKE_PLATFORM_INFO_DIR}/LtoTest1${lang}")
                  file(MAKE_DIRECTORY "${__output_dir}")
                  set(__output_base "${__output_dir}/lto-test-${lang}")

                  execute_process(
                    COMMAND ${CMAKE_COMMAND} -E echo "void foo() {}"
                    COMMAND ${CMAKE_${lang}_COMPILER} ${__lto_flags} -c -xc -
                      -o "${__output_base}.o"
                    RESULT_VARIABLE __result
                    ERROR_QUIET
                    OUTPUT_QUIET
                  )

                  if("${__result}" STREQUAL "0")
                    execute_process(
                      COMMAND ${CMAKE_LTO_AR} cr "${__output_base}.a" "${__output_base}.o"
                      RESULT_VARIABLE __result
                      ERROR_QUIET
                      OUTPUT_QUIET
                    )
                  endif()

                  if("${__result}" STREQUAL "0")
                    execute_process(
                      COMMAND ${CMAKE_LTO_RANLIB} "${__output_base}.a"
                      RESULT_VARIABLE __result
                      ERROR_QUIET
                      OUTPUT_QUIET
                    )
                  endif()

                  if("${__result}" STREQUAL "0")
                    execute_process(
                      COMMAND ${CMAKE_COMMAND} -E echo "void foo(); int main() {foo();}"
                      COMMAND ${CMAKE_${lang}_COMPILER} ${__lto_flags} -xc -
                        -x none "${__output_base}.a" -o "${__output_base}"
                      RESULT_VARIABLE __result
                      ERROR_QUIET
                      OUTPUT_QUIET
                    )
                  endif()

                  if("${__result}" STREQUAL "0")
                    set(__lto_found TRUE)
                  endif()

                  set(CMAKE_${lang}_PASSED_LTO_TEST
                    ${__lto_found} CACHE INTERNAL
                    "If the compiler passed a simple LTO test compile")
                endif()
                if(CMAKE_${lang}_PASSED_LTO_TEST)
                  message(STATUS "Checking for LTO Compatibility - works")
                  set(LTO_${lang}_SUPPORT TRUE CACHE BOOL "Do we have LTO support ?")
                  set(LTO_COMPILE_FLAGS -flto CACHE STRING "Link Time Optimization compile flags")
                  set(LTO_LINK_FLAGS -flto CACHE STRING "Link Time Optimization link flags")
                else()
                  message(STATUS "Checking for LTO Compatibility - not working")
                endif()

              endif()
            elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
                message(STATUS "Checking for LTO Compatibility - works (assumed for clang)")
                set(LTO_${lang}_SUPPORT TRUE CACHE BOOL "Do we have LTO support ?")
                set(LTO_COMPILE_FLAGS -flto CACHE STRING "Link Time Optimization compile flags")
                set(LTO_LINK_FLAGS -flto CACHE STRING "Link Time Optimization link flags")
            elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
                message(STATUS "Checking for LTO Compatibility - works")
                set(LTO_${lang}_SUPPORT TRUE CACHE BOOL "Do we have LTO support ?")
                set(LTO_COMPILE_FLAGS /GL CACHE STRING "Link Time Optimization compile flags")
                set(LTO_LINK_FLAGS -LTCG:INCREMENTAL CACHE STRING "Link Time Optimization link flags")
            else()
                message(STATUS "Checking for LTO Compatibility - compiler not handled by module")
            endif()
            mark_as_advanced(LTO_${lang}_SUPPORT LTO_COMPILE_FLAGS LTO_LINK_FLAGS)


            set(LTO_${lang}_CHECKED TRUE CACHE INTERNAL "" )

            if(CMAKE_GCC_AR AND CMAKE_GCC_RANLIB AND CMAKE_GCC_NM)
                # THIS IS HACKY BUT THERE IS NO OTHER SOLUTION ATM
                set(CMAKE_AR ${CMAKE_GCC_AR} CACHE FILEPATH "Forcing gcc-ar instead of ar" FORCE)
                set(CMAKE_NM ${CMAKE_GCC_NM} CACHE FILEPATH "Forcing gcc-nm instead of nm" FORCE)
                set(CMAKE_RANLIB ${CMAKE_GCC_RANLIB} CACHE FILEPATH "Forcing gcc-ranlib instead of ranlib" FORCE)
            endif()
      endif(${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} VERSION_LESS 3.9)
    endif(OPENPHYSYN_LTO_ENABLED AND NOT LTO_${lang}_CHECKED)


    if(OPENPHYSYN_LTO_ENABLED)
      #Special case for cmake older than 3.9, using a library for gcc/clang, but could setup the flags directly.
      #Taking advantage of the [debug,optimized] parameter of target_link_libraries
      if(${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} VERSION_LESS 3.9)
        if(LTO_${lang}_SUPPORT)
            if(NOT TARGET __enable_lto_tgt)
                add_library(__enable_lto_tgt INTERFACE)
            endif()
            target_compile_options(__enable_lto_tgt INTERFACE ${LTO_COMPILE_FLAGS})
            #this might not work for all platforms... in which case we'll have to set the link flags on the target directly
            target_link_libraries(__enable_lto_tgt INTERFACE ${LTO_LINK_FLAGS} )
            macro(target_enable_lto _target _build_configuration)
                if(${_build_configuration} STREQUAL "optimized" OR ${_build_configuration} STREQUAL "debug" )
                    target_link_libraries(${_target} PRIVATE ${_build_configuration} __enable_lto_tgt)
                else()
                    target_link_libraries(${_target} PRIVATE __enable_lto_tgt)
                endif()
            endmacro()
        else()
            #In old cmake versions, we can set INTERPROCEDURAL_OPTIMIZATION even if not supported by the compiler
            #So if we didn't detect it, let cmake give it a try
            set(__IPO_SUPPORTED TRUE)
        endif()
      else()
          cmake_policy(SET CMP0069 NEW)
          include(CheckIPOSupported)
          # Optional IPO. Do not use IPO if it's not supported by compiler.
          check_ipo_supported(RESULT __IPO_SUPPORTED OUTPUT output)
          if(NOT __IPO_SUPPORTED)
            message(STATUS "IPO is not supported or broken.")
          else()
            message(STATUS "IPO is supported")
          endif()
      endif()
      if(__IPO_SUPPORTED)
        macro(target_enable_lto _target _build_configuration)
            if(NOT ${_build_configuration} STREQUAL "debug" )
                #enable for all configurations
                set_target_properties(${_target} PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
            endif()
            if(${_build_configuration} STREQUAL "optimized" )
                #blacklist debug configurations
                set(__enable_debug_lto FALSE)
            else()
                #enable only for debug configurations
                set(__enable_debug_lto TRUE)
            endif()
            get_property(DEBUG_CONFIGURATIONS GLOBAL PROPERTY DEBUG_CONFIGURATIONS)
            if(NOT DEBUG_CONFIGURATIONS)
                set(DEBUG_CONFIGURATIONS DEBUG) # This is what is done by CMAKE internally... since DEBUG_CONFIGURATIONS is empty by default
            endif()
            foreach(config IN LISTS DEBUG_CONFIGURATIONS)
                set_target_properties(${_target} PROPERTIES INTERPROCEDURAL_OPTIMIZATION_${config} ${__enable_debug_lto})
            endforeach()
        endmacro()
      endif()
    endif()
    if(NOT COMMAND target_enable_lto)
        macro(target_enable_lto _target _build_configuration)
        endmacro()
    endif()
endmacro()
