############################################################################
##
## BSD 3-Clause License
##
## Copyright (c) 2021, The Regents of the University of California
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
##

project(ParquetFP
  LANGUAGES CXX 
)

add_library(ParquetFP
  src/AnalytSolve.cxx
  src/baseannealer.cxx
  src/basepacking.cxx
  src/btreeanneal.cxx
  src/btreecompact.cxx
  src/btree.cxx
  src/btreeslackeval.cxx
  src/ClusterDB.cxx
  src/CommandLine.cxx
  src/DB.cxx
  src/FPcommon.cxx
  src/mixedpacking.cxx
  src/mixedpackingfromdb.cxx
  src/Net.cxx
  src/netlist.cxx
  src/Nets.cxx
  src/Node.cxx
  src/Nodes.cxx
  src/plcompact.cxx
  src/plsptobtree.cxx
  src/pltobtree.cxx
  src/PlToSP.cxx
  src/SeqPair.cxx
  src/skyline.cxx
  src/SolveMulti.cxx
  src/SPeval.cxx
)

target_include_directories(ParquetFP
  PUBLIC
  ${PARQUET_HOME}
  )

target_include_directories(ParquetFP
  PUBLIC
    src
)

