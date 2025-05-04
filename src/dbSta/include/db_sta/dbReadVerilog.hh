// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

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
class dbSta;
class NetworkReader;
}  // namespace sta

namespace ord {

class dbVerilogNetwork;

using odb::dbDatabase;

using sta::Cell;
using sta::ConcreteCell;
using sta::ConcreteNetwork;
using sta::dbNetwork;
using sta::VerilogReader;

// Hierarchical network for read_verilog.
// Verilog cells and module networks are built here.
// It is NOT part of an Sta.
class dbVerilogNetwork : public ConcreteNetwork
{
 public:
  dbVerilogNetwork();
  Cell* findAnyCell(const char* name) override;
  void init(dbNetwork* db_network);
  bool isBlackBox(ConcreteCell* cell);

 private:
  NetworkReader* db_network_ = nullptr;
};

dbVerilogNetwork* makeDbVerilogNetwork();

void initDbVerilogNetwork(dbVerilogNetwork* network, sta::dbSta* sta);

void setDbNetworkLinkFunc(dbVerilogNetwork* network,
                          VerilogReader* verilog_reader);

void deleteDbVerilogNetwork(dbVerilogNetwork* verilog_network);

// Read a hierarchical Verilog netlist into a OpenSTA concrete network
// objects. The hierarchical network is elaborated/flattened by the
// link_design command and OpenDB objects are created from the flattened
// network.
bool dbLinkDesign(const char* top_cell_name,
                  dbVerilogNetwork* verilog_network,
                  dbDatabase* db,
                  utl::Logger* logger,
                  bool hierarchy,
                  bool omit_filename_prop = false);

}  // namespace ord
