###########################################################################
## Original authors: SangGi Do(sanggido@unist.ac.kr), Mingyu Woo(mwoo@eng.ucsd.edu)
##          (respective Ph.D. advisors: Seokhyeong Kang, Andrew B. Kahng)
## Rewrite by James Cherry, Parallax Software, Inc.
##
## BSD 3-Clause License
##
## Copyright (c) 2019, James Cherry, Parallax Software, Inc.
## Copyright (c) 2018, SangGi Do and Mingyu Woo
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
###########################################################################

include("openroad")

swig_lib(NAME         dpo
         NAMESPACE    dpo
         I_FILE       src/UWdp.i
         SCRIPTS      src/UWdp.tcl
)

target_sources(dpo
  PRIVATE
    src/UWdp.cpp
    src/MakeUWdp.cpp
    src/architecture.cxx
    src/network.cxx
    src/router.cxx
    src/utility.cxx
    src/plotgnu.cxx
    src/mer.cxx
    src/legalize.cxx
    src/legalize_dwsm.cxx
    src/legalize_flow_single.cxx
    src/min_movement_floorplanner.cxx
    src/seq_pair.cxx
    src/detailed.cxx
    src/detailed_manager.cxx
    src/detailed_mis.cxx
    src/detailed_interleaving.cxx
    src/detailed_reorder.cxx
    src/detailed_global.cxx
    src/detailed_vertical.cxx
    src/detailed_displacement.cxx
    src/detailed_hpwl.cxx
    src/detailed_abu.cxx
    src/detailed_random.cxx
    src/detailed_orient.cxx
)

target_include_directories(dpo
  PUBLIC
    include
)

target_link_libraries(dpo
  PRIVATE
    odb
    OpenSTA
    utl
    dpl
)

messages(
  TARGET dpo
)

################################################################

add_custom_target(uwdp_tags etags -o TAGS
  ${UWDP_SRC}
  ${UWDP_HOME}/include/dpo/*.h
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
