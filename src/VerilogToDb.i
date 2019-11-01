%module verilog2db

// Verilog2db, Verilog to OpenDB
// Copyright (c) 2019, Parallax Software, Inc.
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

%{

#include "opendb/db.h"
#include "opendb/lefin.h"
#include "opendb/defout.h"
#include "Machine.hh"
#include "Network.hh"
#include "MakeConcreteNetwork.hh"
#include "VerilogReader.hh"
#include "VerilogWriter.hh"
#include "VerilogToDb.hh"

using sta::NetworkReader;
using sta::Debug;
using sta::Report;

// Hierarchical network for read_verilog.
static NetworkReader *verilog_network = nullptr;

NetworkReader *
getVerilogNetwork()
{
  if (verilog_network == nullptr) {
    verilog_network = sta::makeConcreteNetwork();
    sta::Sta *sta = getSta();
    Report *report = sta->report();
    verilog_network->setReport(report);
    Debug *debug = sta->debug();
    verilog_network->setDebug(debug);
  }
  return verilog_network;
}

%}

%inline %{

bool
read_verilog(const char *filename)
{
  NetworkReader *verilog_network = getVerilogNetwork();
  return sta::readVerilogFile(filename, verilog_network);
}

// Write a flat verilog netlist for the database.
void
write_verilog_cmd(const char *filename,
		  bool sort)
{
  sta::writeVerilog(filename, sort, getSta()->network());
}

void
link_design_db_cmd(const char *top_cell_name)
{
  bool link_make_black_boxes = true;
  sta::Sta *sta = getSta();
  NetworkReader *verilog_network = getVerilogNetwork();
  bool success = verilog_network->linkNetwork(top_cell_name,
					      link_make_black_boxes,
					      sta->report());
  if (success) {
    verilog2db(verilog_network, getDb());
    deleteVerilogReader();
  }
}

%} // inline
