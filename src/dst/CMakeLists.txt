###############################################################################
##
## BSD 3-Clause License
##
## Copyright (c) 2021, University of California, San Diego.
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
include("openroad")
include(FindZLIB)
find_package(TCL)

project(dst
  LANGUAGES CXX
)
find_package(Boost REQUIRED COMPONENTS serialization system thread)
swig_lib(NAME      dst
         NAMESPACE dst
         I_FILE    src/Distributed.i
         SCRIPTS   src/Distributed.tcl
)
target_sources(dst
  PRIVATE
  src/JobMessage.cc
  src/Worker.cc
  src/LoadBalancer.cc
  src/WorkerConnection.cc
  src/BalancerConnection.cc
  src/MakeDistributed.cc
  src/Distributed.cc
)

target_include_directories(dst
  PUBLIC
    include
)

target_link_libraries(dst
  PUBLIC
    utl
    OpenSTA
    ${TCL_LIBRARY}
    ${ZLIB_LIBRARIES}
    Boost::serialization
    Boost::system
    Boost::thread
)

messages(
  TARGET dst
)

add_subdirectory(test)
