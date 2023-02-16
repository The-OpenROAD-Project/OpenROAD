///////////////////////////////////////////////////////////////////////////
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

#pragma once

#include <map>
#include <memory>
#include <random>

namespace ord {
class dbVerilogNetwork;
}

namespace odb {
class dbDatabase;
class dbChip;
class dbBlock;
}  // namespace odb

namespace sta {
class dbNetwork;
class Instance;
class NetworkReader;
class Library;
class Port;
class Net;
class dbSta;
}  // namespace sta

namespace utl {
class Logger;
}

using utl::Logger;

namespace par {

class PartitionMgr
{
 public:
  PartitionMgr();
  ~PartitionMgr();
  void init(odb::dbDatabase* db,
            sta::dbNetwork* db_network,
            sta::dbSta* sta,
            Logger* logger);

  // The TritonPart Interface
  // TritonPart is a state-of-the-art hypergraph and netlist partitioner that
  // replaces previous engines such as hMETIS.
  // The TritonPart is designed for VLSI CAD, thus it can understand
  // all kinds of constraints and timing information.
  void tritonPartHypergraph(const char* hypergraph_file,
                            const char* fixed_file,
                            unsigned int num_parts,
                            float balance_constraint,
                            int vertex_dimensions,
                            int hyperedge_dimensions,
                            unsigned int seed);
  // partition netlist
  void tritonPartDesign(unsigned int num_parts,
                        float balance_constraint,
                        unsigned int seed,
                        const std::string& solution_filename,
                        const std::string& paths_filename,
                        const std::string& hypergraph_filename);

  // OpenROAD C++ interface
  // Used by HierRTLMP
  // 2-way partition c++ interface
  std::vector<int> TritonPart2Way(
      int num_vertices,
      int num_hyperedges,
      const std::vector<std::vector<int>>& hyperedges,
      const std::vector<float>& vertex_weights,
      float balance_constraints,
      int seed = 0);

 private:
  odb::dbDatabase* db_ = nullptr;
  sta::dbNetwork* db_network_ = nullptr;
  sta::dbSta* _sta = nullptr;
  Logger* logger_;
};

}  // namespace par
