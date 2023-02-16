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

#include "par/PartitionMgr.h"

#include <time.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>

#include "TritonPart.h"
#include "autocluster.h"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "utl/Logger.h"

using utl::PAR;

namespace par {

PartitionMgr::PartitionMgr() : logger_(nullptr)
{
}

PartitionMgr::~PartitionMgr()
{
}

void PartitionMgr::init(odb::dbDatabase* db,
                        sta::dbNetwork* db_network,
                        sta::dbSta* sta,
                        utl::Logger* logger)
{
  db_ = db;
  db_network_ = db_network;
  _sta = sta;
  logger_ = logger;
}

void PartitionMgr::tritonPartHypergraph(const char* hypergraph_file,
                                        const char* fixed_file,
                                        unsigned int num_parts,
                                        float balance_constraint,
                                        int vertex_dimensions,
                                        int hyperedge_dimensions,
                                        unsigned int seed)
{
  // Use TritonPart to partition a hypergraph
  // In this mode, TritonPart works as hMETIS.
  // Thus users can use this function to partition the input hypergraph
  auto triton_part
      = std::make_unique<TritonPart>(db_network_, db_, _sta, logger_);
  triton_part->tritonPartHypergraph(hypergraph_file,
                                    fixed_file,
                                    num_parts,
                                    balance_constraint,
                                    vertex_dimensions,
                                    hyperedge_dimensions,
                                    seed);
}

void PartitionMgr::tritonPartDesign(unsigned int num_parts,
                                    float balance_constraint,
                                    unsigned int seed,
                                    const std::string& solution_filename,
                                    const std::string& paths_filename,
                                    const std::string& hypergraph_filename)
{
  auto triton_part
      = std::make_unique<TritonPart>(db_network_, db_, _sta, logger_);
  triton_part->tritonPartDesign(num_parts,
                                balance_constraint,
                                seed,
                                solution_filename,
                                paths_filename,
                                hypergraph_filename);
}

std::vector<int> PartitionMgr::TritonPart2Way(
    int num_vertices,
    int num_hyperedges,
    const std::vector<std::vector<int>>& hyperedges,
    const std::vector<float>& vertex_weights,
    float balance_constraints,
    int seed)
{
  auto triton_part
      = std::make_unique<TritonPart>(db_network_, db_, _sta, logger_);
  return triton_part->TritonPart2Way(num_vertices,
                                     num_hyperedges,
                                     hyperedges,
                                     vertex_weights,
                                     balance_constraints,
                                     seed);
}

}  // namespace par
