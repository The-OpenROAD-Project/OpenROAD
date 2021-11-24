/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2019, The Regents of the University of California
// All rights reserved.
//
// BSD 3-Clause License
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
#include "db_sta/dbNetwork.hh"
#include "sta/ConcreteNetwork.hh"
#include "sta/VerilogReader.hh"

namespace utl {
class Logger;
}

namespace odb {
class dbDatabase;
}

namespace sta {
class NetworkReader;
}

namespace ord {

class dbVerilogNetwork;
class OpenRoad;

using odb::dbDatabase;

using sta::Cell;
using sta::ConcreteNetwork;
using sta::dbNetwork;

// Hierarchical network for read_verilog.
// Verilog cells and module networks are built here.
// It is NOT part of an Sta.
class dbVerilogNetwork : public ConcreteNetwork
{
 public:
  dbVerilogNetwork();
  virtual Cell* findAnyCell(const char* name);
  void init(dbNetwork* db_network);

 private:
  NetworkReader* db_network_;
};

dbVerilogNetwork* makeDbVerilogNetwork();

void initDbVerilogNetwork(OpenRoad* openroad);

void deleteDbVerilogNetwork(dbVerilogNetwork* verilog_network);

// Read a hierarchical Verilog netlist into a OpenSTA concrete network
// objects. The hierarchical network is elaborated/flattened by the
// link_design command and OpenDB objects are created from the flattened
// network.
void dbReadVerilog(const char* filename, dbVerilogNetwork* verilog_network);

void dbLinkDesign(const char* top_cell_name,
                  dbVerilogNetwork* verilog_network,
                  dbDatabase* db,
                  utl::Logger* logger);

}  // namespace ord
