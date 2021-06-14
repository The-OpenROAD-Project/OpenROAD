############################################################################
##
## Copyright (c) 2021, OpenROAD
## All rights reserved.
##
## BSD 3-Clause License
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions are met:
##
## * Redistributions of source code must retain the above copyright notice, this
##   list of conditions and the following disclaimer.
##
## * Redistributions in binary form must reproduce the above copyright notice,
##   this list of conditions and the following disclaimer in the documentation
##   and/or other materials provided with the distribution.
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
############################################################################

# Generate the messages.txt file automatically
# Arguments
#   TARGET <target>: the target to post-build trigger from
#   OUTPUT_DIR <dir>: the directory to write the messages.txt in
#   LOCAL: don't recurse

function(messages)

  # Parse args
  set(options LOCAL)
  set(oneValueArgs TARGET OUTPUT_DIR)
  set(multiValueArgs "")

  cmake_parse_arguments(
      ARG  # prefix on the parsed args
      "${options}"
      "${oneValueArgs}"
      "${multiValueArgs}"
      ${ARGN}
  )

  # Validate args
  if (DEFINED ARG_UNPARSED_ARGUMENTS)
     message(FATAL_ERROR "Unknown argument(s) to swig_lib: ${ARG_UNPARSED_ARGUMENTS}")
  endif()

  if (DEFINED ARG_KEYWORDS_MISSING_VALUES)
     message(FATAL_ERROR "Missing value for argument(s) to swig_lib: ${ARG_KEYWORDS_MISSING_VALUES}")
  endif()

  if (NOT DEFINED ARG_TARGET)
    message(FATAL_ERROR "TARGET argument must be provided to messages")
  endif()

  if (NOT DEFINED ARG_OUTPUT_DIR)
    set(ARG_OUTPUT_DIR .)
  endif()

  if (${ARG_LOCAL})
    set(local '--local')
  endif()

  add_custom_command(
    TARGET ${ARG_TARGET}
    POST_BUILD
    COMMAND ${CMAKE_SOURCE_DIR}/etc/find_messages.py
        ${local}
        > ${ARG_OUTPUT_DIR}/messages.txt
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )
endfunction()
