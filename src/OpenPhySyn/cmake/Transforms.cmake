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
set(TRANSFORMS_BUILD_DIR "${CMAKE_BINARY_DIR}/transforms")

file(MAKE_DIRECTORY ${TRANSFORMS_BUILD_DIR})


set(PSN_HOME ${CMAKE_CURRENT_SOURCE_DIR} CACHE FILEPATH "The path to OpenPhySyn includes")
set(PSN_TRANSFORM_INSTALL_PATH "$ENV{HOME}/.OpenPhySyn/transforms" CACHE FILEPATH "Default path for transforms installation")

if (${OPENPHYSYN_TRANSFORM_HELLO_TRANSFORM_ENABLED})
add_subdirectory(src/StandardTransforms/HelloTransform)

if(NOT ${OPENPHYSYN_TRANSFORM_AUTO_LINK_TRANSFORMS})
install(
  TARGETS hello_transform
  DESTINATION ${PSN_TRANSFORM_INSTALL_PATH}
)
endif()

endif()

if (${OPENPHYSYN_TRANSFORM_BUFFER_FANOUT_ENABLED})
add_subdirectory(src/StandardTransforms/BufferFanoutTransform)

if(NOT ${OPENPHYSYN_TRANSFORM_AUTO_LINK_TRANSFORMS})
install(
  TARGETS buffer_fanout
  DESTINATION ${PSN_TRANSFORM_INSTALL_PATH}
)
endif()

endif()

if (${OPENPHYSYN_TRANSFORM_GATE_CLONE_ENABLED})
add_subdirectory(src/StandardTransforms/GateCloningTransform)

if(NOT ${OPENPHYSYN_TRANSFORM_AUTO_LINK_TRANSFORMS})
install(
  TARGETS gate_clone
  DESTINATION ${PSN_TRANSFORM_INSTALL_PATH}
)
endif()

endif()

if (${OPENPHYSYN_TRANSFORM_PIN_SWAP_ENABLED})
add_subdirectory(src/StandardTransforms/PinSwapTransform)

if(NOT ${OPENPHYSYN_TRANSFORM_AUTO_LINK_TRANSFORMS})
install(
  TARGETS pin_swap
  DESTINATION ${PSN_TRANSFORM_INSTALL_PATH}
)
endif()

endif()

if (${OPENPHYSYN_TRANSFORM_CONSTANT_PROPAGATION_ENABLED})
add_subdirectory(src/StandardTransforms/ConstantPropagationTransform)

if(NOT ${OPENPHYSYN_TRANSFORM_AUTO_LINK_TRANSFORMS})
install(
  TARGETS constant_propagation
  DESTINATION ${PSN_TRANSFORM_INSTALL_PATH}
)
endif()

endif()