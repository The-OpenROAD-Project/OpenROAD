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

SUBDIRLIST(TRANSFORMS_DIRS ${CMAKE_SOURCE_DIR}/src/StandardTransforms)

set(PSN_HOME ${CMAKE_CURRENT_SOURCE_DIR} CACHE FILEPATH "The path to OpenPhySyn includes")
set(PSN_TRANSFORM_INSTALL_PATH lib/openphysyn/transforms)
set(PSN_TRANSFORM_INSTALL_FULL_PATH ${CMAKE_INSTALL_PREFIX}/lib/openphysyn/transforms)

# DEFINETRANSFORM should be called to compile any included transform
# For example, to add a new transform use: DEFINETRANSFORM(mytransform MyTransform.cpp MyTransform.hpp)
# You should not need to update this MACRO
MACRO(DEFINETRANSFORM TRANSFORM_NAME SOURCES HEADER CLASSNAME)
  add_library(${TRANSFORM_NAME} ${SOURCES})

  target_link_libraries(${TRANSFORM_NAME}
  PUBLIC
  OpenSTA
  ${OPENPHYSYN_LIBRARY_NAME}
  )

  target_include_directories(${TRANSFORM_NAME} PUBLIC
    src/
    ${PSN_HOME}/include/
    ${PUBLIC_INCLUDE_DIRS}
    ${PRIVATE_INCLUDE_DIRS}
  )
  
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

  if (DEFINED "OPENPHYSYN_TRANSFORMS_BUILD_DIR")
    set_target_properties(${TRANSFORM_NAME}
        PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${OPENPHYSYN_TRANSFORMS_BUILD_DIR}"
    )
  endif()

  target_compile_features(${TRANSFORM_NAME} PRIVATE cxx_range_for cxx_auto_type)
  target_compile_options(${TRANSFORM_NAME} PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wall>)
  set_property(TARGET ${TRANSFORM_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)
  install(
    TARGETS ${TRANSFORM_NAME}
    DESTINATION ${PSN_TRANSFORM_INSTALL_PATH}
  )
  set(OPENPHYSYN_TRANSFORM_TARGETS ${OPENPHYSYN_TRANSFORM_TARGETS} ${TRANSFORM_NAME} PARENT_SCOPE)
  get_filename_component(HEADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/${HEADER} DIRECTORY)
  get_filename_component(HEADER_NAME ${CMAKE_CURRENT_SOURCE_DIR}/${HEADER} NAME)
  set(OPENPHYSYN_TRANSFORM_HEADER_INCLUDES ${OPENPHYSYN_TRANSFORM_HEADER_INCLUDES} ${HEADER_PATH} PARENT_SCOPE)
  set(OPENPHYSYN_TRANSFORM_HEADER_NAMES ${OPENPHYSYN_TRANSFORM_HEADER_NAMES} ${HEADER_NAME} PARENT_SCOPE)
  set(OPENPHYSYN_TRANSFORM_CLASS_NAMES ${OPENPHYSYN_TRANSFORM_CLASS_NAMES} ${CLASSNAME} PARENT_SCOPE)
ENDMACRO()


# Defining and compiling the transforms
if (${OPENPHYSYN_TRANSFORM_HELLO_TRANSFORM_ENABLED})
add_subdirectory(src/StandardTransforms/HelloTransform)
endif()

if (${OPENPHYSYN_TRANSFORM_BUFFER_FANOUT_ENABLED})
add_subdirectory(src/StandardTransforms/BufferFanoutTransform)
endif()

if (${OPENPHYSYN_TRANSFORM_GATE_CLONE_ENABLED})
add_subdirectory(src/StandardTransforms/GateCloningTransform)
endif()

if (${OPENPHYSYN_TRANSFORM_PIN_SWAP_ENABLED})
add_subdirectory(src/StandardTransforms/PinSwapTransform)
endif()

if (${OPENPHYSYN_TRANSFORM_CONSTANT_PROPAGATION_ENABLED})
add_subdirectory(src/StandardTransforms/ConstantPropagationTransform)
endif()

if (${OPENPHYSYN_TRANSFORM_TIMING_BUFFER_ENABLED})
add_subdirectory(src/StandardTransforms/TimingBufferTransform)
endif()

if (${OPENPHYSYN_TRANSFORM_REPAIR_TIMING_ENABLED})
add_subdirectory(src/StandardTransforms/RepairTimingTransform)
endif()


message(STATUS "OpenPhySyn enabled transforms: ${OPENPHYSYN_TRANSFORM_TARGETS}")
set(OPENPHYSYN_TRANSFORMS_INCLUDE_STRING "")

foreach(tran ${OPENPHYSYN_TRANSFORM_HEADER_NAMES})
  set(OPENPHYSYN_TRANSFORMS_INCLUDE_STRING ${OPENPHYSYN_TRANSFORMS_INCLUDE_STRING} "#include \"${tran}\"")
endforeach()
list(JOIN OPENPHYSYN_TRANSFORMS_INCLUDE_STRING "\n" OPENPHYSYN_TRANSFORMS_INCLUDE_STRING)
set(OPENPHYSYN_LOAD_TRANSFORMS_LOOP "")
list(LENGTH OPENPHYSYN_TRANSFORM_TARGETS OPENPHYSYN_TRANSFORM_COUNT)

math(EXPR TRANSFORMS_COUNTER "${OPENPHYSYN_TRANSFORM_COUNT} - 1")


foreach(ind RANGE "${TRANSFORMS_COUNTER}")
list(GET OPENPHYSYN_TRANSFORM_TARGETS ${ind} NAME)
list(GET OPENPHYSYN_TRANSFORM_CLASS_NAMES ${ind} CLS)
set(OPENPHYSYN_LOAD_TRANSFORMS_LOOP ${OPENPHYSYN_LOAD_TRANSFORMS_LOOP} "auto trans${ind} = std::make_shared<${CLS}>()")
set(OPENPHYSYN_LOAD_TRANSFORMS_LOOP ${OPENPHYSYN_LOAD_TRANSFORMS_LOOP} "CONTAINER[\"${NAME}\"] = trans${ind}")
set(OPENPHYSYN_LOAD_TRANSFORMS_LOOP ${OPENPHYSYN_LOAD_TRANSFORMS_LOOP} "INFO[\"${NAME}\"] =TransformInfo(\"${NAME}\", trans${ind}->help(), trans${ind}->version(), trans${ind}->description())")
set(OPENPHYSYN_LOAD_TRANSFORMS_LOOP ${OPENPHYSYN_LOAD_TRANSFORMS_LOOP} "COUNTER++")
endforeach()
list(JOIN OPENPHYSYN_LOAD_TRANSFORMS_LOOP ";" OPENPHYSYN_LOAD_TRANSFORMS_LOOP)


foreach(tran ${OPENPHYSYN_TRANSFORM_TARGETS})
  set(PUBLIC_LIBRARIES
      ${PUBLIC_LIBRARIES}
      ${tran})
endforeach()

configure_file (
  "${PROJECT_SOURCE_DIR}/cmake/Config.hpp.in"
  "${PROJECT_BINARY_DIR}/Config.hpp"
)