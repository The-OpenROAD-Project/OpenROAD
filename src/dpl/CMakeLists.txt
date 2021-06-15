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

swig_lib(NAME         dpl
         NAMESPACE    dpl
         I_FILE       src/Opendp.i
         SCRIPTS      src/Opendp.tcl
)

target_sources(dpl
  PRIVATE
    src/Opendp.cpp
    src/dbToOpendp.cpp
    src/MakeOpendp.cpp
    src/Grid.cpp
    src/CheckPlacement.cpp
    src/Place.cpp
    src/FillerPlacement.cpp
    src/OptMirror.cpp
)

target_include_directories(dpl
  PUBLIC
    include
)

target_link_libraries(dpl
  PRIVATE
    opendb
    OpenSTA
    utl
)

messages(
  TARGET dpl
)

################################################################

add_custom_target(opendp_tags etags -o TAGS
  ${OPENDP_SRC}
  ${OPENDB_HOME}/include/dpl/*.h
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
