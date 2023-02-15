/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, The Regents of the University of California
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////////

%{
#include "par/PartitionMgr.h"

namespace ord {
// Defined in OpenRoad.i
        par::PartitionMgr* getPartitionMgr();
}  // namespace ord

using ord::getPartitionMgr;
%}

%include "../../Exception.i"

%inline %{

void triton_part_hypergraph(const char* hypergraph_file, const char* fixed_file,
                             unsigned int num_parts, float balance_constraint,
                             int vertex_dimension, int hyperedge_dimension,
                             unsigned int seed)
{
  getPartitionMgr()->tritonPartHypergraph(hypergraph_file, fixed_file,
                                           num_parts, balance_constraint, 
                                           vertex_dimension, hyperedge_dimension,
                                           seed);
}

void triton_part_design(unsigned int num_parts,
                        float balance_constraint,
                        unsigned int seed,
                        const char* solution_filename,
                        const char* paths_filename,
                        const char* hypergraph_filename)
{
  getPartitionMgr()->tritonPartDesign(num_parts,
                                      balance_constraint,
                                      seed,
                                      solution_filename,
                                      paths_filename,
                                      hypergraph_filename);
}


%}
