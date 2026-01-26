// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once
#include "db_sta/dbNetwork.hh"
#include "odb/db.h"
#include "sta/ConcreteLibrary.hh"
#include "sta/ConcreteNetwork.hh"
#include "sta/NetworkClass.hh"
#include "sta/VerilogReader.hh"

namespace utl {
class Logger;
}

namespace sta {
class dbSta;
class NetworkReader;
}  // namespace sta

namespace ord {

class dbVerilogNetwork;

// Hierarchical network for read_verilog.
// Verilog cells and module networks are built here.
// It is NOT part of an Sta.
class dbVerilogNetwork : public sta::ConcreteNetwork
{
 public:
  dbVerilogNetwork(sta::dbSta* sta);
  sta::Cell* findAnyCell(const char* name) override;
  bool isBlackBox(sta::ConcreteCell* cell);
  sta::dbNetwork* getDbNetwork()
  {
    return static_cast<sta::dbNetwork*>(db_network_);
  }

 private:
  NetworkReader* db_network_ = nullptr;
};

void setDbNetworkLinkFunc(dbVerilogNetwork* network,
                          sta::VerilogReader* verilog_reader);

// Read a hierarchical Verilog netlist into a OpenSTA concrete network
// objects. The hierarchical network is elaborated/flattened by the
// link_design command and OpenDB objects are created from the flattened
// network.
bool dbLinkDesign(const char* top_cell_name,
                  dbVerilogNetwork* verilog_network,
                  odb::dbDatabase* db,
                  utl::Logger* logger,
                  bool hierarchy,
                  bool omit_filename_prop = false);

}  // namespace ord
